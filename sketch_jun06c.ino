#include <SocketIoClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
SocketIoClient client;
const char* ssid = "Redmiphong";
const char* password = "123456789";
char host[] = "192.168.43.76";
int port = 3000;
extern String RID;
extern String Rfull;
unsigned long previousMillis = 0;
long interval = 2000;
int32_t red_raw = 0; // 4
int32_t red_moving_average = 0; //4
int32_t red_unfiltered = 0; // 4
int32_t red_filtered = 0; // 4
int32_t ir_raw = 0; // 4
int32_t ir_moving_average = 0; //4
int32_t ir_unfiltered = 0; // 4
int32_t ir_filtered = 0; // 4
uint8_t beatDetected = 0;
uint8_t beat_per_minute = 0;
uint32_t timestamp = 0;
uint8_t spo2 = 0;
char Data[200];
char out[255];
char character;
int i = 0;
void setup()
{
  Serial.begin(115200);
  delay(10);
  Serial.print("Ket noi vao mang ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { //Thoát ra khỏi vòng
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  Serial.println(F("Da ket noi WiFi"));
  Serial.println(F("Dia chi IP cua ESP8266 (Socket Client ESP8266): "));
  Serial.println(WiFi.localIP());
  client.begin("192.168.43.76", 3000);
}
void loop()
{
  while(Serial.available() )
  {
    character = Serial.read();
  //  Serial.print(character);
    //Data.concat(character);
    if (character == '\n')
    {
        sscanf( Data, "%d:%d:%d:%d:%d:%d:%d:%d:%u:%u:%u:%u", &red_raw,&red_moving_average,&red_unfiltered,&red_filtered,&ir_raw,&ir_moving_average,&ir_unfiltered,&ir_filtered,&beatDetected,&beat_per_minute,&spo2,&timestamp);
        sprintf(out, "{\"redraw\":\"%d\",\"redavg\":\"%d\",\"reduf\":\"%d\",\"redf\":\"%d\",\"irraw\":\"%d\",\"iravg\":\"%d\",\"iruf\":\"%d\",\"irf\":\"%d\",\"bd\":\"%u\",\"bpm\":\"%u\",\"spo2\":\"%u\",\"time\":\"%u\"}\r\n",\
                                                                    red_raw,red_moving_average,red_unfiltered,red_filtered,ir_raw,ir_moving_average,ir_unfiltered,ir_filtered,beatDetected,beat_per_minute,spo2,timestamp);
        i = 0;   
        client.emit("push data", out);   
        Data[0] = '\0';
        Serial.flush();
        
    } else {
      Data[i++] =  character;
    }
  }
  delay(500);
  client.loop();
}
