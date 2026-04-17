#include <Arduino.h>
#include <WiFiMulti.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <SoftwareSerial.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <SPI.h>    
#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "TFT_eSPI.h" /* Please use the TFT library provided in the library. */
#include "img_logo.h"
#include "pin_config.h"
#include <esp_adc_cal.h>
#include "USB.h"
#include "USBCDC.h"

/* The product now has two screens, and the initialization code needs a small change in the new version. The LCD_MODULE_CMD_1 is used to define the
 * switch macro. */
#define LCD_MODULE_CMD_1

// Battery calibration in mV
#define MAX_BATT 4048
#define MIN_BATT 2900

#define WAKE_UP_PIN GPIO_NUM_14
#define ON_OFF GPIO_NUM_0

// Definições do Debounce
#define Debounce 100 // intervalo ms para evitar erro no contador
unsigned long lastInterruptTime = 0; // Último tempo da interrupção
unsigned long debounceDelay = 1000;    // Tempo de debounce em ms

// Variável para controlar se o ESP32 está ativo ou deve entrar em deep sleep
bool deviceAwake = true;  // Começa como ativo

TFT_eSPI tft = TFT_eSPI();
#define WAIT 1000
unsigned long targetTime = 0; // Used for testing draw times

bool idlle = 0;
bool standBy = 0;

#if defined(LCD_MODULE_CMD_1)
typedef struct {
    uint8_t cmd;
    uint8_t data[14];
    uint8_t len;
} lcd_cmd_t;

lcd_cmd_t lcd_st7789v[] = {
    {0x11, {0}, 0 | 0x80},
    {0x3A, {0X05}, 1},
    {0xB2, {0X0B, 0X0B, 0X00, 0X33, 0X33}, 5},
    {0xB7, {0X75}, 1},
    {0xBB, {0X28}, 1},
    {0xC0, {0X2C}, 1},
    {0xC2, {0X01}, 1},
    {0xC3, {0X1F}, 1},
    {0xC6, {0X13}, 1},
    {0xD0, {0XA7}, 1},
    {0xD0, {0XA4, 0XA1}, 2},
    {0xD6, {0XA1}, 1},
    {0xE0, {0XF0, 0X05, 0X0A, 0X06, 0X06, 0X03, 0X2B, 0X32, 0X43, 0X36, 0X11, 0X10, 0X2B, 0X32}, 14},
    {0xE1, {0XF0, 0X08, 0X0C, 0X0B, 0X09, 0X24, 0X2B, 0X22, 0X43, 0X38, 0X15, 0X16, 0X2F, 0X37}, 14},
};
#endif

//#define DEBUG
#define CLOUD
#define RANDOM

//BLE
#define SERVICE_UUID "ab0828b1-198e-4351-b779-901fa0e0371e"
#define CHARACTERISTIC_UUID_RX "4ac8a682-9736-4e5d-932b-e9b31405049c"
#define CHARACTERISTIC_UUID_TX "0972EF8C-7613-4075-AD52-756F33D4DA91"

//RS232 Serial
#define RXD2                      18  // RX2 ESP32
#define TXD2                      17  // TX2 ESP32

// Comando RS232 device para leitura de resultados
#define COMANDO                   "L"

//WifiLED
#define wifiLed   PIN_LCD_BL   //D2
#define BLINK_INTERVAL 500 

//RESULT LED
#define BLINK_INTERVAL_RESULT 250

//Tempo de inatividade para desconexão do BLE, em segundos
#define BLE_TIMEOUT               120 // Após este tempo, se nenhuma leitura for realizada, faz a desconexão do BLE

//Qtde de tentativas de leitura do instrumento antes de reiniciar
#define RD_TRY                    10 // após estas tentativas, reiniciar por erro de retorno do instrumento

//Nome Instrumento Serial
#define DEVICE_INSTRUMENT_ID      "IoT_SensorLV_Balance" // ID do instrumento que vai se conectar à serial

//#IoT Serial Number
#define N_SERIAL                  "LV_IOT#568-1235" // Número serial único ITF para o device do contrato

//ANALISES
#define CUSTOMER_NAME   "-LV"
#define ANALISE_NAME    "Weighing"
#define UNIT            "G"
#define STATUS          "READING OK"
#define CLOUD_STATUS    "SENT TO LIMS OK"

// To TAGO-IO
String apiKey                   = "8961a44f-ce78-41b4-aa45-8d28caaee714";       // O token vai aqui
const char* serverTago          = "api.tago.io";
const char* serverName01        = "https://api.tago.io/data?variable=atuador01&qty=1";
char valueOk[]                  = {"ATIVADO"};
int totalHeader                 = 1;
const char * headerKeys[]       = {"8961a44f-ce78-41b4-aa45-8d28caaee714"} ;
const size_t numberOfHeaders    = 1;

// To time
unsigned long timeNow         = 0;                    //  tempo em milisegundos para temporizador de loops de leitura
unsigned long  initialMillis  = 0;

// To Time e NTP
char timeStamp[50];
char timeData[20];
char timeHour[20];
char timeID[20];
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * -3;
const int   daylightOffset_sec = 0;



// Variáveis de controle
float i = 0;
int j = 0;  // contador de tempos de inatividade
int w = 0; // contador de tentativas de leitura do instrumento
std::string rxValue = "";
String valorRes = "";
char txString[8];
void flushSerial2();
bool deviceConnected = false; //Controle de dispositivo conectado
int ledState = LOW;   // ledState used to set the LED
unsigned long previousMillisBlink = 0; 
int teste=0;
//Serial OTG Comunication
float receivedValue = 0.0;  // Armazenará o valor recebido pela serial
char sampleIdReceived[15]; //x-xxxxxx-xxxxx
bool newValueReceived = false;  // Flag para indicar novo valor
USBCDC USBSerial2;

void processOTGSerial() {
  if (USBSerial2.available() > 0) {
    String input = USBSerial2.readStringUntil('\n');
    input.trim();

    // Converte String para char[] para usar strtok
    char inputBuffer[50];
    input.toCharArray(inputBuffer, sizeof(inputBuffer));

    // Tokeniza com base na vírgula
    char* token = strtok(inputBuffer, ",");
    if (token != NULL) {
      float newValue = atof(token); // primeiro valor como float

      // Pega o segundo token
      token = strtok(NULL, ",");
      if (token != NULL) {
        strncpy(sampleIdReceived, token, sizeof(sampleIdReceived) - 1);
        sampleIdReceived[sizeof(sampleIdReceived) - 1] = '\0'; // garante terminação nula

        // Validação do número (zero é válido)
        if (newValue != 0 || strcmp(token, "0") == 0 || strcmp(token, "0.0") == 0) {
          receivedValue = newValue;
          newValueReceived = true;

          // Atualiza o display
          tft.setRotation(1);
          tft.fillScreen(TFT_BLACK);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.drawString("Captured from Balance", 30, 20, 4);
          tft.drawString(ANALISE_NAME, 114, 50, 4);
          tft.drawString(String(receivedValue), 105, 90, 6);

          Serial.print("Novo valor recebido pela OTG: ");
          Serial.println(receivedValue);
          Serial.print("Sample ID recebido: ");
          Serial.println(sampleIdReceived);

  
          dtostrf(receivedValue, 7, 2, txString);
          sendTAGO();
        }
      }
    }
  }
}




//Inicia objeto BLE
BLECharacteristic *characteristicTX; //Através desse objeto iremos enviar dados para o client

// Inicia WiFi e client
WiFiMulti wifiMulti;
WiFiClient client;
WiFiClientSecure client1;

//Callback para eventos das características. Configurado pra printar o valor no serial quando receber um dado
class CharacteristicCallbacks: public BLECharacteristicCallbacks {
  
    void onWrite(BLECharacteristic *characteristic) {
      //Retorna ponteiro para o registrador contendo o valor atual da caracteristica
      rxValue = characteristic->getValue();
      //Verifica se existe dados (tamanho maior que zero)

      if (rxValue.length() > 0) {
        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }
        Serial.println("valor disponivel: ");
      

        if (rxValue == "L") {

          // String conteudo = "";
          // char caractere;

          // if (Serial2.available() <= 0) {
          //   Serial.println("Comando de leitura recebido");
          // }
          // //Serial2.write(0x05);
          // //Serial2.write(0x05);
          // if (Serial2.available() > 0) {
          //   Serial.println("Status OK");
          //   while (Serial2.available()) {
          //     // Lê byte da serial
          //     caractere = char(Serial2.read());
              
          //     // INSERIR NESTE IF TODOS OS CARACTERES A SEREM ELIMINADOS DA CONCATENAÇÃO DO RESULTADO
          //     if (caractere != '\n' && caractere != '\r' && caractere != 'S' && caractere != ' ' && caractere != 'g') {
          //       // Concatena valores
          //       conteudo.concat(caractere);
          //     }
          //     // Aguarda buffer serial ler próximo caractere
          //     delay(10);
          //   }
          //   valorRes = conteudo;
          //   Serial.print("Leitura: ");
            
          //   Serial.println(valorRes);
          //   //rxValue = "";
          //   //Serial.print("rxValue: ");
          //   //Serial.print(rxValue[i]);
          //   //flushSerial2();
          // }
          // else {
          //   //Comando Serie Mettler Toledo XP50x
          //   //Serial2.write(0x53); 

            
          //   Serial2.write(COMANDO);  
          //   Serial2.println();
          //   Serial.println("Requisição Enviada");
          //   Serial.println(COMANDO);
          //   w = w + 1;
          //   delay (750);

            //RANDOM
            #ifdef RANDOM
            valorRes = String(receivedValue);
            Serial.println(valorRes);
            #endif

            unsigned long currentMillis = millis();
            
          }
          
          delay (250);
        }

      }
    };

//Callback para receber os eventos de conexão de dispositivos
class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      tft.setRotation(1);
      tft.setTextSize(1);
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString ("Device Connected!", 0, 20, 2);
      tft.drawString ("Waiting new reading...", 40, 60, 4);
      Serial.println("Device Connected!");
      Serial.println();
      digitalWrite(wifiLed , HIGH);
      j=0;

    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      digitalWrite(wifiLed , LOW);
      j=0;
      tft.fillScreen(TFT_BLACK);
      //tft.drawString ("Desconected. Restarting...", 0, 0, 2);
      Serial.println ("Reiniciando IoT por desconexão do usuário");
      delay(1000);
      ESP.restart();
    }
};

// void flushSerial2() {

//   while (Serial2.available() > 0) {
//     char t = Serial2.read();
//   }

//   Serial.println("Buffer serial reiniciado");
//   Serial.println("-------------------------");
  

// }

void setupBLE(){
  
    //Setup BLE
  BLEDevice::init(DEVICE_INSTRUMENT_ID); //Nome do dispositivo
  
  BLEServer *server = BLEDevice::createServer(); //Cria um BLE Server
  
  server->setCallbacks(new ServerCallbacks()); //Seta o callback do server
  
  BLEService *service = server->createService(SERVICE_UUID); //Cria o BLE Service
  
  characteristicTX = service->createCharacteristic(CHARACTERISTIC_UUID_TX,
                     BLECharacteristic::PROPERTY_NOTIFY); //Cria o BLE Characteristic para envio de dados
  characteristicTX->addDescriptor(new BLE2902());

  BLECharacteristic *characteristic = service->createCharacteristic(CHARACTERISTIC_UUID_RX,
                                      BLECharacteristic::PROPERTY_WRITE); //Cria o BLE Characteristic para recebimento de dados
  characteristic->setCallbacks(new CharacteristicCallbacks());

  service->start(); //Inicia o service
  
  server->getAdvertising()->start(); //Inicia o advertising (descoberta do ESP)
  
}

void setupWifi(){

    //unsigned long currentMillis = millis();

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);


    wifiMulti.addAP("LelisTwibi", "Dr@cma231099#$%");
    wifiMulti.addAP("LelisForce", "Dr@cma231099#$%");
    wifiMulti.addAP("VIVOFIBRA-LELIS", "Dr@cma231099#$%");
    wifiMulti.addAP("Lox", "Dr@cma231099#$%");
    //wifiMulti.addAP("LabVantage", "CTEC2024!");
    //wifiMulti.addAP("HotelIn9_stella", "hotelin9stella");
    
    Serial.println(F("Connecting Wifi..."));

    // if (wifiMulti.run() != WL_CONNECTED) {
    // tft.fillScreen(TFT_BLACK);
    // tft.setTextColor(TFT_WHITE, TFT_BLACK);
    // tft.drawString ("Wifi Connected!", 0, 20, 2);
    // tft.drawString (String(WiFi.SSID()), 40, 60, 4);
    // }

    //delay (250);

}

void setupNTP()
{

  //Configura e Inicia Tempo
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  //  timeClient.begin();
  //  timeClient.setTimeOffset(offsetNTP);

  #ifdef  DEBUG
  Serial.println("Setup Tempo e NTP ok! ");
  Serial.println();
  #endif

} // end setupNTP

void setupTFT(){

  tft.begin();

#if defined(LCD_MODULE_CMD_1)
    for (uint8_t i = 0; i < (sizeof(lcd_st7789v) / sizeof(lcd_cmd_t)); i++) {
        tft.writecommand(lcd_st7789v[i].cmd);
        for (int j = 0; j < (lcd_st7789v[i].len & 0x7f); j++) {
            tft.writedata(lcd_st7789v[i].data[j]);
        }

        if (lcd_st7789v[i].len & 0x80) {
            delay(120);
        }
    }
#endif
     
    tft.setRotation(1);
    tft.setSwapBytes(true);

    if (deviceAwake) {
      //Mostra Logos
      tft.pushImage(0, 0, 320, 170, (uint16_t *)gImage_LVLogo);
      delay(2000);

      tft.pushImage(0, 0, 320, 170, (uint16_t *)gImage_MobileAppLogo);
      //delay(2000);
      
    }

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5,0,0)
    ledcSetup(0, 2000, 8);
    ledcAttachPin(PIN_LCD_BL, 0);
    ledcWrite(0, 255);
#else
    ledcAttach(PIN_LCD_BL, 200, 8);
    ledcWrite(PIN_LCD_BL, 255);
#endif
}

void enterDeepSleep() {
  Serial.println("Entrando em Deep Sleep. Pressione o botão para acordar...");
  
  // Configura o pino como fonte de interrupção para acordar
  esp_sleep_enable_ext0_wakeup(WAKE_UP_PIN, 0);  // Acorda quando o pino for LOW (botão pressionado)

  tft.fillScreen(TFT_BLACK);
  digitalWrite(PIN_POWER_ON, LOW);

  // Entra no modo deep sleep
  deviceAwake = false;
  esp_deep_sleep_start();
}

void setup() {

  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);

  pinMode(WAKE_UP_PIN, INPUT);
  pinMode(ON_OFF, INPUT_PULLUP);

  Serial.begin(115200);
  Serial.println("Iniciando IoT device");

  setupTFT();

  //Inicia RS232
  //Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  //delay(1500);

  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, HIGH);

  setupBLE();
  delay(2000);
  USB.begin();
  USBSerial2.begin();

  #ifdef  CLOUD
  setupWifi();
  setupNTP();
  #endif

  //delay(2000);

  Serial.println("Setup IoT OK");
  Serial.println(DEVICE_INSTRUMENT_ID);

  // // Reinicia RS232 por motivo de estabilidade pós start/reset
  // Serial2.end();
  // delay(1500);
  // Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  // delay(1500);
  // flushSerial2();
  
}

void loop() {

  // if (millis() > targetTime) {
    
  //       esp_adc_cal_characteristics_t adc_chars;

  //       // Get the internal calibration value of the chip
  //       esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  //       uint32_t raw = analogRead(PIN_BAT_VOLT);
  //       uint32_t v1 = esp_adc_cal_raw_to_voltage(raw, &adc_chars) * 2; //The partial pressure is one-half

  //       float v1_perc = 100 - ( (( (MAX_BATT - v1)*100) ) / (MAX_BATT - MIN_BATT) ); // Percentage calculation based on battery bias MAX and MIN charge

  //        button.tick();

  //      if (digitalRead(keybat) == 1) {

  //      tft.fillScreen(TFT_BLACK);
  //       tft.setCursor(5, 10);

  //       // If the battery is not connected, the ADC detects the charging voltage of TP4056, which is inaccurate.
  //       // Can judge whether it is greater than 4300mV. If it is less than this value, it means the battery exists.
  //       if (v1 > 4300) {
  //           tft.println("Charging");
  //           tft.print(v1);
  //           tft.print("mV");
  //       } else {
  //           tft.print(v1_perc);
  //           tft.print("%");
  //           tft.println();
  //           tft.print(v1);
  //           //tft.print("V");
  //           tft.print("mV");float v1_perc = 100 - ( (( (MAX_BATT - v1)*100) ) / (MAX_BATT - MIN_BATT) ); // Percentage calculation based on battery bias MAX and MIN charge
  //       }

  //     }

  //       targetTime = millis() + 1000;
  //     }
  //   delay(20);
  processOTGSerial();
   // Verifica se o botão foi pressionado (nível baixo)
  if (digitalRead(WAKE_UP_PIN) == LOW) {
    delay(50);  // Debouncing básico

    // // Verifica se o botão ainda está pressionado após o debounce
    if (digitalRead(WAKE_UP_PIN) == LOW) {
      delay(100); // Adiciona uma pausa de 500ms para evitar múltiplos cliques rápidos

      // Se o dispositivo está ativo, entra em deep sleep
      if (deviceAwake) {
        //deviceAwake = false;  // Marca que o dispositivo está dormindo
        enterDeepSleep();  // Entra em deep sleep
      }
    }
  }

  #ifdef  CLOUD
  if (wifiMulti.run() != WL_CONNECTED) {
    setupWifi();
   }
  #endif

  //Se algum dispositivo está conectado e um comando de leitura foi enviado, monta  string de resultado para enviar
  if (deviceConnected && rxValue == "L" && valorRes != "" && txString != "" && deviceAwake
  
  #ifdef  CLOUD
  && wifiMulti.run() == WL_CONNECTED
  #endif
  
  ) {

    Serial.println("Comando  foi recebido");

    unsigned long currentMillis = millis();

      if (currentMillis - previousMillisBlink >= BLINK_INTERVAL_RESULT) { 
      ledState = (ledState == LOW) ? HIGH : LOW;
      digitalWrite(wifiLed , ledState);
      previousMillisBlink = currentMillis;
      }
    
    Serial.println("Entrou no loop...");
    //dtostrf(i, 2, 2, txString);
    valorRes.toCharArray(txString, 16);
    characteristicTX->setValue(txString);
    characteristicTX->notify();
    Serial.print("Valor enviado: ");
    Serial.println(txString);
    Serial.print("txString: ");
    Serial.println(rxValue[i]);
    Serial.print("valorRes: ");
    Serial.println(valorRes);
    Serial.print("rxValue: ");
    Serial.println(rxValue[i]);
    Serial.println();
    //flushSerial2(); // limpa o buffer de transmissão da serial RS232
    

    //Escreve no display
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString (String(STATUS), 80, 20, 4);
    tft.drawString (String(ANALISE_NAME), 80, 50, 4);
    tft.drawString (String(txString), 100, 90, 6);
    tft.drawString (String(UNIT), 200, 100, 4);

    //Pisca LED Resultado 2x
    digitalWrite(wifiLed , LOW);
    delay(250);
    digitalWrite(wifiLed , HIGH);
    delay(250);
    digitalWrite(wifiLed , LOW);
    delay(250);
    digitalWrite(wifiLed , HIGH);
    delay(250);

    if(deviceConnected){
      digitalWrite(wifiLed , HIGH);
    } else {
      digitalWrite(wifiLed , LOW);
    }

    #ifdef  CLOUD
    readNTP();
    sendTAGO();   // Envia ao TAGO IO
    #endif

    Serial.println("Limpar variáveis...");
    rxValue = "";
    valorRes = "";
    txString[0] = 0;
    
    //j = 0; // zera o contador de tempo de inatividade

  } 
  
  //else {

    //Antes da primeira leitura, ou após um evento de timeout/reset, reiniciar a serial
    // if (j == 0 && deviceConnected == false) {
    //   Serial2.end();
    //   delay(1500);
    //   Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
    //   delay(1500);
    //   flushSerial2();
    //   Serial.println ("Porta Serial OK");
    //   Serial.println("Aguardando dispositivo conectar");
    // }

    //Após o tempo de timeout configurado de inatividade, reiniciar o dispositivo em algumas situações

    // if (j == (BLE_TIMEOUT *0.8) && deviceConnected == false){ // compara o contador com a constante de timeout por inatividade de conexão
    //     Serial.println ("Reiniciando IoT por inatividade de conexão de " + String((int((BLE_TIMEOUT*0.8)/4))/ 60) + " minuto");
    //     delay(1000);
    //     ESP.restart();
    // } 

    // if (j == (BLE_TIMEOUT * 4) && deviceConnected){ // compara o contador com a constante de timeout por inatividade de leitura
    //     Serial.println ("Reiniciando IoT por inatividade de leitura de " + String((int(BLE_TIMEOUT))/ 60) + " minutos");
    //     delay(1000);
    //     ESP.restart();
    // } 

    // if (w == RD_TRY && deviceConnected) {
    //    Serial.println ("Reiniciando IoT por erro de envio dados do Instrumento após " + String(RD_TRY) + " tentativas");
    //    delay(1000);
    //    ESP.restart();
    // }
    
  //delay(250);

  //j = j + 1;
  //Serial.println(String(j));
}

// TFT Pin check
    #if PIN_LCD_WR          != TFT_WR  || \
        PIN_LCD_RD          != TFT_RD  || \
        PIN_LCD_CS          != TFT_CS  || \
        PIN_LCD_DC          != TFT_DC  || \
        PIN_LCD_RES         != TFT_RST || \
        PIN_LCD_D0          != TFT_D0  || \
        PIN_LCD_D1          != TFT_D1  || \
        PIN_LCD_D2          != TFT_D2  || \
        PIN_LCD_D3          != TFT_D3  || \
        PIN_LCD_D4          != TFT_D4  || \
        PIN_LCD_D5          != TFT_D5  || \
        PIN_LCD_D6          != TFT_D6  || \
        PIN_LCD_D7          != TFT_D7  || \
        PIN_LCD_BL          != TFT_BL  || \
        TFT_BACKLIGHT_ON    != HIGH    || \
        170   != TFT_WIDTH  || \
        320   != TFT_HEIGHT
#error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#endif

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)
#error  "The current version is not supported for the time being, please use a version below Arduino ESP32 3.0"
#endif

void sendTAGO()
{

  #ifdef DEBUG
  Serial.println("Conectando a TAGO.IO...");
  #endif

  if (client.connect(serverTago, 80))
  {

    #ifdef DEBUG
    Serial.println("Conectado ao server LV_MobIoT_01");
    #endif


    //*****************************************************
    // DEFINIÇÃO DO TAGO HTTP POST HEADER e API TOKEN
    //*****************************************************

    String postStr = "";
    String postData = "";

    postStr = "POST /data  HTTP/1.1\n";
    postStr += "Host: api.tago.io\n";
    postStr += String("Device-Token: ") + apiKey + "\n";
    postStr += "_ssl: false\n";
    postStr += String("Content-Type: ") + "application/json" + "\n";

    // Inicia o array de múltiplas variáveis para o JSON TAGO e segue construindo as variáveis do postData
    postData = "[";

    //*****************************************************
    // VAR 1 - ID
    //*****************************************************

    postData += "{\"variable\":\"id\",\"value\":\""; // quando variável é texto
    postData += String(timeID);
    postData += "\"}";  // quando variável é texto

    postData += ","; // separador para a próxima variável

    //*****************************************************
    // VAR 2 - DATA
    //*****************************************************

    postData += "{\"variable\":\"date\",\"value\":\""; // quando variável é texto
    postData += String(timeData);
    postData += "\"}";  // quando variável é texto

    postData += ","; // separador para a próxima variável

    //*****************************************************
    // VAR 3 - HORA
    //*****************************************************

    postData += "{\"variable\":\"hour\",\"value\":\""; // quando variável é texto
    postData += String(timeHour);
    postData += "\"}";  // quando variável é texto

    postData += ","; // separador para a próxima variável

    //*****************************************************
    // VAR 4 - TEXT_ID
    //*****************************************************

    postData += "{\"variable\":\"textid\",\"value\":\""; // quando variável é texto
    postData += String(ANALISE_NAME);
    postData += String(CUSTOMER_NAME);
    postData += String("-");
    postData += String(timeID);
    postData += "\"}";  // quando variável é texto

    postData += ","; // separador para a próxima variável

    //*****************************************************
    // VAR 5 -ANALISE
    //*****************************************************

    postData += "{\"variable\":\"analysis\",\"value\":\""; // quando variável é texto
    postData += String(ANALISE_NAME);
    postData += "\"}";  // quando variável é texto

    postData += ","; // separador para a próxima variável

    //*****************************************************
    // VAR 6 - RESULTADO
    //*****************************************************

    postData += "{\"variable\":\"result\",\"value\":"; // quando variável é número
    postData += String(txString);
    postData += "}";  // quando variável é número

    postData += ","; // separador para a próxima variável

    //*****************************************************
    // VAR 7 - UNIDADE
    //*****************************************************

    postData += "{\"variable\":\"unit\",\"value\":\""; // quando variável é texto
    postData += String(UNIT);
    postData += "\"}";  // quando variável é texto

    postData += ","; // separador para a próxima variável

        //*****************************************************
    // VAR 8 - RESPONSAVEL
    //*****************************************************

    postData += "{\"variable\":\"resp\",\"value\":\""; // quando variável é texto
    postData += String("SYS_IOT");
    postData += "\"}";  // quando variável é texto

    postData += ","; // separador para a próxima variável

    //*****************************************************
    // VAR 9 - STATUS
    //*****************************************************

    postData += "{\"variable\":\"status\",\"value\":\""; // quando variável é texto
    postData += String(STATUS);
    postData += "\"}";  // quando variável é texto

    //*****************************************************
    // FINALIZA O JSON DE VARIAVEIS
    //*****************************************************
    postData += "]"; // fecha o array

    //Concatena o final do POST
    postStr += String("Content-Length: ") + String(postData.length()) + "\n";
    postStr += "\n";
    postStr += postData;

    client.print(postStr); // Realiza o post no cliente TAGO

     unsigned long timeout = millis();

    while (client.available() == 0)
    {

      if (millis() - timeout > 5000)
      {

        client.stop();
        return;

        #ifdef  DEBUG
        Serial.println(">>> Client Timeout !");
        Serial.println();
        #endif

      } // end if

    } // end while

    #ifdef  DEBUG
    while (client.available())
    {
      String line = client.readStringUntil('\r');
      Serial.print("String e: ");
      Serial.print(line);
      Serial.println();

    } // end while
    #endif

    Serial.println(postData);
    Serial.println("Dados enviados ao Server TAGO!");
    tft.drawString (String(CLOUD_STATUS), 35, 140, 4);
    Serial.println();
    client.stop();

    #ifdef DEBUG
    Serial.print("ID: ");
    Serial.print(timeID);
    Serial.println();
    Serial.print("DATA: ");
    Serial.print(timeData);
    Serial.println();
    Serial.print("Hora ");
    Serial.print(timeHour);
    Serial.println();
    Serial.print("Análise: ");
    Serial.print(ANALISE_NAME);
    Serial.println();
    Serial.print("Resultado: ");
    Serial.print(String(valorRes) + String(UNIT));
    Serial.println();
    Serial.print("Status: ");
    Serial.print(String(STATUS));
    Serial.println();

    //delay(1000);
    #endif

  } // end if

  client.stop();

} // end sendTAGO

//*****************************************************************************************************************
// Function to read NTP
//*****************************************************************************************************************

void readNTP()
{

  //while(!timeClient.update()) { timeClient.forceUpdate(); }

  //Concatena estrutura de Data e Hora
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  strftime(timeStamp, sizeof(timeStamp), "%d-%m-%Y %H:%M:%S", &timeinfo);
  strftime(timeData, sizeof(timeData), "%d-%m-%Y" , &timeinfo); // DATA
  strftime(timeHour, sizeof(timeHour), "%H:%M:%S" , &timeinfo); // HORA
  strftime(timeID, sizeof(timeID), "%d%m%Y%H%M%S%", &timeinfo); // ID
  //resID = String(timeID) + String(random(0,999));
  
  Serial.println(String(timeStamp));

  //timeStamp       = timeClient.getFormattedTime();

  #ifdef  DEBUG
  Serial.print("Data e Hora: ");
  Serial.println(timeStamp);
  #endif

} // end readNTP
