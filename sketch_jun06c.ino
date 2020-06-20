#include <SocketIoClient.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
SocketIoClient client;
SoftwareSerial mySerial(13, 15); // RX, TX d7, d8
const char* ssid = "Vi Phong";
const char* password = "vi1524phong1727";
char host[] = "remote-health-biometric.herokuapp.com";
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
char originalTime[100];
uint32_t originCpuTime = 0;
uint8_t spo2 = 0;
char Data[200];
char out[255];
char character;
boolean isDaytime = false;
int i = 0;
void event(const char * payload, size_t length) {
  sscanf( payload, "%s", originalTime);
//  Serial.printf("got message: %s\n", payload);
//  Serial.printf("got message: %s\n", originalTime);
  isDaytime = true; 
}
void setup()
{
  mySerial.begin(115200);
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
  client.begin("remote-health-biometric.herokuapp.com", 80);
  //client.begin("192.168.43.157", 3000);
  client.emit("request time");
  client.on("return time", event);
  while (!isDaytime) {
    yield();
    client.loop();
//    mySerial.print('.');
  }
}


void loop()
{
  while(Serial.available() )
  {
    character = Serial.read();
    if (character == '\n')
    {
        Data[i++] = '\0';
        int count_scan_successfully = sscanf( Data, "%d:%d:%d:%d:%d:%d:%d:%d:%hhu:%hhu:%hhu:%u", 
        &red_raw,
        &red_moving_average,
        &red_unfiltered,
        &red_filtered,
        &ir_raw, 
        &ir_moving_average, 
        &ir_unfiltered, 
        &ir_filtered, 
        &beatDetected, 
        &beat_per_minute, 
        &spo2, 
        &timestamp
        );
        Serial.println(count_scan_successfully);
        if (count_scan_successfully != 12) break;
//        sprintf(out, "{\"redraw\":\"%d\",\"redavg\":\"%d\",\"reduf\":\"%d\",\"redf\":\"%d\",\"irraw\":\"%d\",\"iravg\":\"%d\",\"iruf\":\"%d\",\"irf\":\"%d\",\"bd\":\"%u\",\"bpm\":\"%u\",\"spo2\":\"%u\",\"time\":\"%u\"}\n",\
//                                                                    red_raw,red_moving_average,red_unfiltered,red_filtered,ir_raw,ir_moving_average,ir_unfiltered,ir_filtered,beatDetected,beat_per_minute,spo2,timestamp);
        if (!originCpuTime) originCpuTime = timestamp;
        sprintf(out, "{\"device_uuid\": \"3c8f06ad-75df-4734-84d8-898938eb4ba4\", \"payload\": {\"Red\": %d, \"IR\": %d, \"BPM\": %u, \"spO2\": %u, \"originTimestamp\": \"%s\", \"cpuTimestamp\": %u}}", red_raw, ir_raw, beat_per_minute, spo2, originalTime, timestamp - originCpuTime);
        i = 0;  
        client.emit("push data", out);  
        //client.loop();
        Serial.println(out);
        Data[0] = '\0';
        Serial.flush();
    } else {
      Data[i++] =  character;
    }
  }
  client.loop();
}
