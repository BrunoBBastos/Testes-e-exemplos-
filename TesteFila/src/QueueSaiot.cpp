// Integrar a funcionalidade de armazenar os dados em fila ao Saiot
// Enviar os dados computados ao sistema

#include <Arduino.h>
#include <QueueList.h> // Fila <<< SUBSTITUIR PELA STD QUEUE
#include <Ticker.h> // Timer
#include <SaIoTDeviceLib.h> // Saiot

#define PUSH_INTERVAL 2 // Intervalo entre pacotes de pulsos
#define SEND_INTERVAL 4 // Intervalo entre envios
#define SENSOR_PIN D3   // Porta do sensor

struct data // Conjunto de informações a serem enviadas
{
  int value;
  unsigned long time;
};

volatile int pulseCounter = 0; // Contador de pulsos do sensor

unsigned long debounceDelay = 500; // Intervalo entre leituras válidas de cada pulsos

bool wifiSt = false; // Estado da conexão WIFI(teste)
bool ready = false;  // Indica se há dados a serem enviados

QueueList<data> dataQ; // Lista FIFO

Ticker PushData;
Ticker SendQ;

WiFiClient espClient;
SaIoTDeviceLib hidrometro("Hidrometro","230519Tst","ricardodev@email.com");
SaIoTSensor medidorAgua("hd01","hidrometro_01","Litros","number");
String senha = "12345678910";
void callback(char* topic, byte* payload, unsigned int length);

void pulseRead()
{                                        // Leitura de pulsos do sensor
  static unsigned long lastDebounce = 0; // Registra a última marcação de tempo
  unsigned long debounce = millis();     // Tempo atual
  if (debounce - lastDebounce > debounceDelay)
  { // Se intervalo entre chamadas for maior que intervalo de leituras válidas
    pulseCounter++;
    Serial.print("\t \t \t +1 \t"); // (Teste)
    Serial.println(debounce);
  }
  lastDebounce = debounce; // Reseta a última marcação de tempo
}

void pushQ()
{ // Preenche a fila
  if (pulseCounter > 0)
  { // Apenas se houverem dados
    data d;
    d.value = pulseCounter;
    d.time = millis() / 1000; // (Teste) Pegar a hora do SAIOT <<< CORRIGIR
    dataQ.push(d);            // Armazena os dados na lista
    pulseCounter = 0;         // Reseta contador
    ready = true;             // Indica existência de dados prontos para envio
  }
}

void popQ()
{
  if (ready && !wifiSt) // (Teste)
    Serial.println("waiting for wifi");
  if (wifiSt && ready)
  { // Se houverem dados prontos e WIFI disponível
    while (dataQ.count() > 0)
    {                          // Enquanto houverem elementos na fila
      data out = dataQ.pop();  // Retira o primeiro dado da fila
      Serial.print(out.value); // Envia
      Serial.print('\t');
      Serial.println(out.time);
    }
    ready = false; // Reseta a flag
  }
}

void sendData2Saiot(){                        // Enviar dados
  String dateTime = SaIoTCom::getDateNow();
 
  medidorAgua.sendData((distanceToNPITI),dateTime);
}

void setup()
{
  pinMode(SENSOR_PIN, INPUT_PULLUP); // Configura porta do sensor

  hidrometro.addSensor(medidorAgua);
  hidrometro.preSetCom(espClient, callback);    // Função padrão obrigatória (?)
  hidrometro.start(senha);  

  Serial.begin(115200); // (Teste)
  Serial.println();
  Serial.println("Comecou");
  // Associa interrupção ext ao pino do sensor
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), pulseRead, RISING);

  PushData.attach(PUSH_INTERVAL, pushQ); // Inicializa timer de armazenamento 
  SendQ.attach(SEND_INTERVAL, popQ); // Inicializa timer de envio
}

void loop()
{
  if (Serial.available() > 0) // (Teste) Envie 'w' pela serial para simular o "status do wifi"
  {
    char c = Serial.read();
    if (c == 'w')
      wifiSt = !wifiSt;
  }

  sendData2Saiot();
  hidrometro.handleLoop();

}
