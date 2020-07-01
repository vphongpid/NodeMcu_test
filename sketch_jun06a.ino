#include <SocketIoClient.h>
//#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <Adafruit_GFX.h>   
#include <Adafruit_ST7735.h>  
#include <Streaming.h>
#include <SPI.h>
#include <Vector.h>
#include <Arduino.h>
#define DEVICE_UUID "3c8f06ad-75df-4734-84d8-898938eb4ba4"
#define NUM_PAYLOAD_SERVER_LIMIT 25
#define UNIT_40MS_UUID 3
#define WIDTH_UUID 36
#define MOD_VALUE_SCREEN_PIXEL 2500
#define TIME_UNIT 8
#define ORGIN_RAW 110
#define HEIGHT_COORDINATE_AXIS  100
#define MAX_DATA_GRAPH 128/TIME_UNIT + 1
#define TFT_RST   D4      
#define TFT_CS    D3      
#define TFT_DC    D8      
//SCK (CLK) ---> NodeMCU pin D5 (GPIO14)
//MOSI(DIN) ---> NodeMCU pin D7 (GPIO13)
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
SocketIoClient client;
const char* ssid = "Vi Phong1";
const char* password = "vi1524phong1727";
unsigned long previousMillis = 0;
int32_t red_raw = 0;  
int32_t red_moving_average = 0; 
int32_t red_unfiltered = 0;  
int32_t red_filtered = 0;  
int32_t ir_raw = 0;  
int32_t ir_moving_average = 0; 
int32_t ir_unfiltered = 0;  
int32_t ir_filtered = 0;  
uint8_t beatDetected = 0;
uint8_t beat_per_minute = 0;
uint32_t timestamp = 0;
char originalTime[100];
uint32_t originCpuTime = 0;
char Data[255] = {0};
char out[255] = {0};
char payloadBT[255] = {0};
char payloadServer[3200] = {0};
String text = DEVICE_UUID;
String old = DEVICE_UUID;
int preY = 0;
int i = 0;
int arrANew[MAX_DATA_GRAPH];
Vector<int> vecParamANew (arrANew);
int arrAOld[MAX_DATA_GRAPH];
Vector<int> vecParamAOld (arrAOld);
int arrBNew[MAX_DATA_GRAPH];
Vector<int> vecParamBNew (arrBNew);
int arrBOld[MAX_DATA_GRAPH];
Vector<int> vecParamBOld (arrBOld);
uint8_t spo2 = 0;
char character;
int countPayload = 0;
boolean isDaytime = false;
int offset = 0;
int currentTime = 0;
int btTime = 0;
int32_t pre_red = 0;
int32_t pre_ir = 0;
boolean isClear = false;
int max_graph_A = MAX_DATA_GRAPH - 1;
int max_graph_B = MAX_DATA_GRAPH - 1;
void drawGraph();
void redrawReceivedInfo(int32_t ir_raw, int32_t red_raw, uint8_t bpm, uint8_t spo2);
void redrawDeviceUuid();
void event(const char * payload, size_t length);

void setup()
{
  //ESP.wdtDisable();
  Serial.begin(115200);
  delay(10);
  Serial.println(MAX_DATA_GRAPH);
  //Use this initializer if you're using a 1.8" TFT
  tft.initR(INITR_BLACKTAB);   //Initialize a ST7735S chip, black tab

  tft.fillScreen(ST7735_BLACK);   //Fill screen with black

  tft.setRotation(0);
  tft.setTextWrap(false); 
  tft.drawRect(0, 115, 30, 20, ST7735_CYAN);  // Draw rectangle (x,y,width,height,color)
  tft.drawRoundRect(38, 115, 90, 20, 10, ST7735_CYAN);  
  tft.setCursor(5, 122);  
  tft.setTextColor(ST7735_WHITE);   //Set color of text. First is the color of text and after is color of background
  tft.setTextSize(1);
  tft.println("BPM");
  tft.drawRect(0, 140, 30, 20, ST7735_CYAN);   
  tft.drawRoundRect(38, 140, 90, 20, 10, ST7735_CYAN); 
  tft.setCursor(3, 147); 
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);   
  tft.println("SPO2");
  
  for (int k = 0; k < MAX_DATA_GRAPH; k++) {
    vecParamANew.push_back(0);
    vecParamAOld.push_back(0);
    vecParamBNew.push_back(0);
    vecParamBOld.push_back(0);
  }
  WiFi.begin(ssid, password);
  Serial.print("Ket noi vao mang ");
  tft.setCursor(0, 0);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.setTextColor(ST7735_WHITE);
    tft.setTextSize(1);
    tft.print('.');
    Serial.print('.');
    delay(1000);
  }
  tft.fillRect(0, 0, 128, 20, ST7735_BLACK);

  tft.setCursor(0, 0);
  tft.setTextColor(ST7735_WHITE);   
  tft.setTextSize(1);
  tft.println(F("Da ket noi WiFi"));
  Serial.println(F("Da ket noi WiFi"));
  Serial.println(F("Dia chi IP cua ESP8266 (Socket Client ESP8266): "));
  Serial.println(WiFi.localIP());
  delay(1000);
  tft.fillRect(0, 0, 128, 20, ST7735_BLACK);

  client.begin("remote-health-biometric.herokuapp.com", 80);
  //client.begin("192.168.43.157", 3000);
  client.emit("request time");
 
  client.on("return time", event);
  while(!isDaytime) {
    client.loop();
    yield();
  }
  tft.setCursor(0, 0);
  tft.setTextColor(ST7735_WHITE);   
  tft.setTextSize(1);
  tft.println(originalTime);
  delay(1000);
  Serial.printf("got message: %s\n", originalTime);
  tft.fillRect(0, 0, 128, 20, ST7735_BLACK);
}

void loop(){
  while (Serial.available()) {
    character = Serial.read();
    
    if (character == '\n'){
      Data[i++] = '\0';
      char temp_arr[255];
      int count_scan_successfully = sscanf( Data, "%d:%d:%d:%d:%d:%d:%d:%d:%hhu:%hhu:%hhu:%u%s",
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
                                            &timestamp,
                                            temp_arr
                                          );

      if (count_scan_successfully != 12) {
        Serial.println("--------------------------------------------------------");
        Serial.println("Data fail");
        Serial.println(Data);
        Serial.println("--------------------------------------------------------");
        Data[0] = '\0';
        i = 0;
        max_graph_A = MAX_DATA_GRAPH - 1;
        max_graph_B = MAX_DATA_GRAPH - 1;
        return;
      }
      
      drawGraph();
      
      redrawReceivedInfo(ir_raw, red_raw, beat_per_minute, spo2);

      redrawDeviceUuid();

      // Send to server  and bluetooth
      if (!originCpuTime) originCpuTime = timestamp;
      
      sprintf(payloadBT, "{\"device_uuid\": \"%s\", \"payload\": [{\"Red\": %d, \"IR\": %d, \"BPM\": %u, \"spO2\": %u, \"originTimestamp\": \"%s\", \"cpuTimestamp\": %u}]}\n",
              DEVICE_UUID,
              red_raw,
              ir_raw,
              beat_per_minute,
              spo2,
              originalTime,
              timestamp - originCpuTime);
      sprintf(out, "{\"Red\": %d, \"IR\": %d, \"BPM\": %u, \"spO2\": %u, \"originTimestamp\": \"%s\", \"cpuTimestamp\": %u}",
              red_raw,
              ir_raw,
              beat_per_minute,
              spo2,
              originalTime,
              timestamp - originCpuTime);
      
      if (!countPayload) sprintf(payloadServer, "{\"device_uuid\": \"%s\", \"payload\": [", DEVICE_UUID);
      strcat(payloadServer, out);
      if (countPayload < NUM_PAYLOAD_SERVER_LIMIT - 1) strcat(payloadServer, ", ");
      else {
        strcat(payloadServer, "]}");
//        Serial.println(payloadServer);
//        client.emit("push data", payloadServer);
      }
      countPayload = (countPayload + 1) % (NUM_PAYLOAD_SERVER_LIMIT);
      if (millis() - btTime >= 1000) {
        Serial.print(payloadBT);
        btTime= millis();
      }
      Data[0] = '\0';
      i = 0;
      max_graph_A = MAX_DATA_GRAPH - 1;
      max_graph_B = MAX_DATA_GRAPH - 1;
    }
    else {
      if (max_graph_A > 0) {
          tft.drawLine(max_graph_A * TIME_UNIT, ORGIN_RAW - vecParamAOld[max_graph_A], (max_graph_A - 1)*TIME_UNIT, ORGIN_RAW - vecParamAOld[max_graph_A - 1], ST7735_BLACK);
          vecParamAOld[max_graph_A] = vecParamANew[max_graph_A];
          tft.drawLine(max_graph_A * TIME_UNIT, ORGIN_RAW - vecParamANew[max_graph_A], (max_graph_A - 1)*TIME_UNIT, ORGIN_RAW - vecParamANew[max_graph_A - 1], ST7735_RED);
          max_graph_A--;
          
      } else {
        vecParamAOld[0] = vecParamANew[0];
        if (max_graph_B > 0) {
          tft.drawLine(max_graph_B * TIME_UNIT, ORGIN_RAW - vecParamBOld[max_graph_B], (max_graph_B - 1)*TIME_UNIT, ORGIN_RAW - vecParamBOld[max_graph_B - 1], ST7735_BLACK);
          vecParamBOld[max_graph_B] = vecParamBNew[max_graph_B];
          tft.drawLine(max_graph_B * TIME_UNIT, ORGIN_RAW - vecParamBNew[max_graph_B], (max_graph_B - 1)*TIME_UNIT, ORGIN_RAW - vecParamBNew[max_graph_B - 1], ST7735_GREEN);
          max_graph_B--;
        } else {
          vecParamBOld[0] = vecParamBNew[0];
        }
      }
      Data[i++] = character;
    }
  }
  client.loop();
}

void event(const char * payload, size_t length) {
  sscanf( payload, "%s", originalTime);
  isDaytime = true;
}

void redrawReceivedInfo(int32_t ir_raw, int32_t red_raw, uint8_t bpm, uint8_t spo2) {
  tft.setCursor(43, 122);
  tft.setTextColor(ST7735_BLACK);  // Set color of text. First is the color of text and after is color of background
  tft.setTextSize(1);  // Set text size. Goes from 0 (the smallest) to 20 (very big)
  tft.print(pre_ir);

  tft.setCursor(43, 147);
  tft.setTextColor(ST7735_BLACK);  // Set color of text. First is the color of text and after is color of background
  tft.setTextSize(1);  // Set text size. Goes from 0 (the smallest) to 20 (very big)
  tft.print(pre_red);

  pre_red = red_raw;
  pre_ir = ir_raw;
  // It draws from the location to down-right
  tft.setCursor(43, 122);  // Set position (x,y)
  tft.setTextColor(ST7735_WHITE);  // Set color of text. First is the color of text and after is color of background
  tft.setTextSize(1);  // Set text size. Goes from 0 (the smallest) to 20 (very big)
  tft.print(ir_raw);

  // It draws from the location to down-right
  tft.setCursor(43, 147);  // Set position (x,y)
  tft.setTextColor(ST7735_WHITE);  // Set color of text. First is the color of text and after is color of background
  tft.setTextSize(1);  // Set text size. Goes from 0 (the smallest) to 20 (very big)
  tft.print(red_raw); 
}

void redrawDeviceUuid() {
  static int count = 0;
  count = (count + 1) % UNIT_40MS_UUID;
  if (count < UNIT_40MS_UUID - 1) return;
  //  Show UUID to tft LCD
  String t = "";

  for (int k = 0; k < WIDTH_UUID; k++)
    t += text.charAt((offset + k) % text.length());

  // Print the string for this iteration
  tft.setCursor(0, 0); // display will be halfway down screen
  tft.setTextColor(ST7735_BLACK);  // Set color of text. First is the color of text and after is color of background
  tft.setTextSize(1);
  tft.print(old);
  tft.setCursor(0, 0); // display will be halfway down screen
  tft.setTextColor(ST7735_WHITE);  // Set color of text. First is the color of text and after is color of background
  tft.setTextSize(1);
  tft.print(t);

  tft.setTextWrap(false); // Don't wrap text to next line

  if (offset < text.length() - 1) {
    offset++;
  } else {
    offset = 0;
  }
  old = t;
}

void drawGraph() {
  vecParamANew.remove(0);
  vecParamBNew.remove(0);
  if (ir_raw <= MOD_VALUE_SCREEN_PIXEL * HEIGHT_COORDINATE_AXIS + 1 && ir_raw >= 0) {
    vecParamANew.push_back(ir_raw / MOD_VALUE_SCREEN_PIXEL);
  } else if (ir_raw > MOD_VALUE_SCREEN_PIXEL * HEIGHT_COORDINATE_AXIS) {
    vecParamANew.push_back(HEIGHT_COORDINATE_AXIS);
  } else {
    vecParamANew.push_back(0);
  }
  if (red_raw <= MOD_VALUE_SCREEN_PIXEL * HEIGHT_COORDINATE_AXIS + 1 && red_raw >= 0) {
    vecParamBNew.push_back(red_raw / MOD_VALUE_SCREEN_PIXEL);
  } else if (red_raw > MOD_VALUE_SCREEN_PIXEL * HEIGHT_COORDINATE_AXIS) {
    vecParamBNew.push_back(HEIGHT_COORDINATE_AXIS);
  } else {
    vecParamBNew.push_back(0);
  }
}
