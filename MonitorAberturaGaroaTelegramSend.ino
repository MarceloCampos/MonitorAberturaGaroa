/* 
 *  MonitorAberturaGaroaTelegramSend.ino
 *  Initial Author: Marcelo Campos - Garoa Hacker Clube - www.github.com/marcelocampos
 *  
 *  Versão 0.1 - 01/Nov/2020 - Inicial 
 * 
 *  Hardware: 
 *    ESP32S
 *  
 *  Dependências:
 *   
 *     
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>

#define USE_SERIAL Serial
// ESP32 pinos:
#define Rele 14      // futuro
#define LED 2
#define PortSensor 5 // ou: 0 para o IO 0 que é o do Bootoloader do ESP
#define qtdAps 3

WiFiMulti wifiMulti;
HTTPClient http;

bool ledStatus = false;

char ssid[qtdAps][32] = {"garoa", "dummy 1", "dummy 2"};  // -|_máx 31 caracteres ou aumente     
char ssid_passw[qtdAps][32] = {"", "senha 1", "senha 2"}; // -|

String strChatId = "";  // Numero do Chat (Chat ID)
String strBotKey = "";  // entre aqui o key do bot            

void setup() 
{
  int apCount = 0;
  
  pinMode(LED,OUTPUT);  
  pinMode(Rele,OUTPUT);
  pinMode(PortSensor,INPUT_PULLUP);
  
  digitalWrite(LED,HIGH);
  digitalWrite(Rele,LOW);
  
  Serial.begin(57600);                                           
  
  Serial.println(F("Garoa Hacker Clube !"));     

  for(uint8_t t = 4; t > 0; t--) 
  {
      digitalWrite(LED, ledStatus);
      USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
      USE_SERIAL.flush();
      delay(750);  
      ledStatus = !ledStatus;       
  }

  while(/*millis() < 600000*/ true )  // tenta infinitamente aqui até conseguir
  {
    wifiMulti.addAP(ssid[apCount], ssid_passw[apCount]);
    USE_SERIAL.println(" [HTTP] WiFi tentando conexao: " + String(ssid[apCount]));

    if((wifiMulti.run() == WL_CONNECTED)) 
    {
        USE_SERIAL.println(" [HTTP] WiFi Conectado " + String(ssid[apCount]));
        USE_SERIAL.println(" [HTTP] IP: " + WiFi.localIP().toString() );
        break;
    } 
    
    if(apCount++ >= qtdAps)
       apCount = 0;
  }
  
  digitalWrite(LED,LOW);
}


void loop() 
{
  bool isPortOpen = false;
    
  unsigned long tLedTick = 1000;      
  unsigned long t_Tick = 0;
  unsigned long t_SensorAct = 0;
 
  String strToSend;
  strToSend.reserve(256); 

  strToSend = "Garoa Hacker Clube Sistema Aviso Abertura Ligado";
  
  TelegramSend(strToSend, "");
  
  while(true)
  {   
    if( millis() - t_Tick >= tLedTick  )
    {
      digitalWrite(LED, ledStatus); 
      ledStatus = !ledStatus;       
      t_Tick = millis();
    }   

    if( digitalRead(PortSensor) == HIGH && isPortOpen )       // sensor Aberto ?
    {
      if( t_SensorAct == 0 )
      {
        t_SensorAct = millis();
      }
      else if (millis() - t_SensorAct >= 1000)
      { 
        strToSend = "Garoa Hacker Clube Aberto ! :-)"; 
        isPortOpen = false;
        USE_SERIAL.println(" Porta Aberta");
        TelegramSend(strToSend, "https://garoa.net.br/w/images/Garoa_aberto.png");
        tLedTick = 250;
        t_SensorAct = 0;
      }
    }
    else if ( digitalRead(PortSensor) == LOW && !isPortOpen ) // sensor fechado ?
    {
      if( t_SensorAct == 0 )
      {
        t_SensorAct = millis();
      }
      else if (millis() - t_SensorAct >= 1000)
      { 
        strToSend = "Garoa Hacker Clube Fechado :-(";  
        isPortOpen = true;
        USE_SERIAL.println(" Porta Fechada");
        TelegramSend(strToSend, "https://garoa.net.br/w/images/Garoa_fechado.png");
        tLedTick = 1000;
        t_SensorAct = 0;
      }
    }
    else if( (digitalRead(PortSensor) == LOW && isPortOpen && t_SensorAct > 0 ) ||  (digitalRead(PortSensor) == HIGH && !isPortOpen && t_SensorAct > 0 ))
    {
      t_SensorAct = 0;  // Debouncing ativo!
    }       
  } 
  Serial.println(F("\n**Oooopscaiu aqui ?**\n"));
}

void TelegramSend(String msgStrToSend, String imgUrlToSend)
{
  String finalStrToSend;
  String baseStrToSend;
  
  USE_SERIAL.print("[HTTP] TelegramSend(): begin...\n");

  if(imgUrlToSend != "")  // envia com imagem ?
  {
    baseStrToSend = "https://api.telegram.org/" + strBotKey + "/sendMessage?chat_id=" + strChatId + "&parse_mode=markdown&text=[​​​​​​​​​​​](" + imgUrlToSend + ")";
  }
  else  // senão envia só texto
  {
    baseStrToSend = "https://api.telegram.org/" + strBotKey + "/sendMessage?chat_id=" + strChatId + "&text=";      
  }
  
  finalStrToSend = String(baseStrToSend + msgStrToSend);
  
  finalStrToSend.replace(" ", "%20");
  Serial.println(finalStrToSend);
  
  http.begin(finalStrToSend);

  USE_SERIAL.print("[HTTP] GET...\n");

  int httpCode = http.GET();

  if(httpCode > 0) 
  {
      USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

      if(httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          USE_SERIAL.println(payload);
      }
  } 
  else 
  {
      USE_SERIAL.printf("[HTTP] GET... falhou, Erro# : %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}
