#include <SocketIoClient.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <Adafruit_GFX.h>  // Include core graphics library
#include <Adafruit_ST7735.h>  // Include Adafruit_ST7735 library to drive the display
#include <Streaming.h>
#include <Vector.h>
#include <Arduino.h>
#define DEVICE_UUID "3c8f06ad-75df-4734-84d8-898938eb4ba4"
#define NUM_PAYLOAD_SERVER_LIMIT 25
#define UNIT_40MS_UUID 3
#define WIDTH_UUID 36
#define MOD_VALUE_SCREEN_PIXEL 2500
#define TIME_UNIT 4
#define ORGIN_RAW 110
#define HEIGHT_COORDINATE_AXIS  100
// ST7735 TFT module connections
#define TFT_RST   D4     // TFT RST pin is connected to NodeMCU pin D4 (GPIO2)
#define TFT_CS    D3     // TFT CS  pin is connected to NodeMCU pin D4 (GPIO0)
#define TFT_DC    D2     // TFT DC  pin is connected to NodeMCU pin D4 (GPIO4)
// initialize ST7735 TFT library with hardware SPI module
// SCK (CLK) ---> NodeMCU pin D5 (GPIO14)
// MOSI(DIN) ---> NodeMCU pin D7 (GPIO13)
// Create display:
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
SocketIoClient client;
const char* ssid = "Vi Phong1";
const char* password = "vi1524phong1727";
unsigned long previousMillis = 0;
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
char Data[255] = {0};
char out[255] = {0};
char payloadBT[255] = {0};
char payloadServer[3200] = {0};
String text = DEVICE_UUID;
int preY = 0;
int i = 0;
int yNew[33];
Vector<int> vectorNew (yNew);
int yOld[33];
Vector<int> vectorOld (yOld);
uint8_t spo2 = 0;
char character;
int countPayload = 0;
boolean isDaytime = false;
int offset = 0;
int currentTime = 0;
int btTime = 0;


void event(const char * payload, size_t length) {
  sscanf( payload, "%s", originalTime);
  tft.setCursor(0, 0);
  tft.setTextColor(ST7735_WHITE);  // Set color of text. First is the color of text and after is color of background
  tft.setTextSize(1);
  tft.println(originalTime);
  delay(1000);
  tft.fillRect(0, 0, 128, 20, ST7735_BLACK);
  isDaytime = true;
}

void setup()
{
  //  ESP.wdtDisable();
  Serial.begin(115200);
  delay(10);
  // Display setup:

  // Use this initializer if you're using a 1.8" TFT
  tft.initR(INITR_BLACKTAB);  // Initialize a ST7735S chip, black tab

  tft.fillScreen(ST7735_BLACK);  // Fill screen with black

  //tft.setRotation(0);  // Set orientation of the display. Values are from 0 to 3. If not declared, orientation would be 0,
  // which is portrait mode.

  tft.setTextWrap(false);  // By default, long lines of text are set to automatically “wrap” back to the leftmost column.
  // To override this behavior (so text will run off the right side of the display - useful for
  // scrolling marquee effects), use setTextWrap(false). The normal wrapping behavior is restored
  // with setTextWrap(true).
  // Write to the display the text "STM32":
  // Draw rectangle:
  tft.drawRect(0, 115, 30, 20, ST7735_CYAN);  // Draw rectangle (x,y,width,height,color)
  // It draws from the location to down-right

  // Draw rounded rectangle:
  tft.drawRoundRect(38, 115, 90, 20, 10, ST7735_CYAN);  // Draw rounded rectangle (x,y,width,height,radius,color)
  // It draws from the location to down-right
  tft.setCursor(5, 122);  // Set position (x,y)
  tft.setTextColor(ST7735_WHITE);  // Set color of text. First is the color of text and after is color of background
  tft.setTextSize(1);  // Set text size. Goes from 0 (the smallest) to 20 (very big)
  tft.println("BPM");
  // Draw rectangle:
  tft.drawRect(0, 140, 30, 20, ST7735_CYAN);  // Draw rectangle (x,y,width,height,color)
  // It draws from the location to down-right
  // Draw rounded rectangle:
  tft.drawRoundRect(38, 140, 90, 20, 10, ST7735_CYAN);  // Draw rounded rectangle (x,y,width,height,radius,color)
  // It draws from the location to down-right
  tft.setCursor(3, 147);  // Set position (x,y)
  tft.setTextColor(ST7735_WHITE);  // Set color of text. First is the color of text and after is color of background
  tft.setTextSize(1);  // Set text size. Goes from 0 (the smallest) to 20 (very big)
  tft.println("SPO2");
  
  for (int k = 0; k < 33; k++) {
    vectorNew.push_back(0);
    vectorOld.push_back(0);
  }
  WiFi.begin(ssid, password);
  tft.setCursor(0, 0);
  while (WiFi.status() != WL_CONNECTED) { //Thoát ra khỏi vòng
    delay(500);
    tft.setTextColor(ST7735_WHITE);  // Set color of text. First is the color of text and after is color of background
    tft.setTextSize(1);
    tft.print('.');
    delay(1000);
  }
  tft.fillRect(0, 0, 128, 20, ST7735_BLACK);

  tft.setCursor(0, 0);
  tft.setTextColor(ST7735_WHITE);  // Set color of text. First is the color of text and after is color of background
  tft.setTextSize(1);
  tft.println(F("Da ket noi WiFi"));
  delay(1000);
  tft.fillRect(0, 0, 128, 20, ST7735_BLACK);

  client.begin("remote-health-biometric.herokuapp.com", 80);
  //client.begin("192.168.43.157", 3000);
  client.emit("request time");
  client.on("return time", event);
  while (!isDaytime) {
    yield();
    client.loop();
  }
}


void loop()
{
  while (Serial.available() ) {
    character = Serial.read();
    if (character == '\n') {
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

      drawGraph();
      
      redrawReceivedInfo(ir_raw, red_raw, beat_per_minute, spo2);

      redrawDeviceUuid();

      //  Send to server  and bluetooth
      if (!originCpuTime) {
        originCpuTime = timestamp;
      }
      sprintf(payloadBT, "{\"device_uuid\": \"%s\", \"payload\": [{\"Red\": %d, \"IR\": %d, \"BPM\": %u, \"spO2\": %u, \"originTimestamp\": \"%s\", \"cpuTimestamp\": %u}]}\0",
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
        client.emit("push data", payloadServer);
        client.loop();
      }
      countPayload = (countPayload + 1) % (NUM_PAYLOAD_SERVER_LIMIT);
      if (millis() - btTime >= 1000) {
        Serial.println(payloadBT);
        btTime= millis();
      }
      //Serial.println(timestamp - originCpuTime);
      Data[0] = '\0';
      //sprintf(out, "{\"device_uuid\": \"3c8f06ad-75df-4734-84d8-898938eb4ba4\", \"payload\": {\"Red\": %d, \"IR\": %d, \"BPM\": %u, \"spO2\": %u, \"originTimestamp\": \"%s\", \"cpuTimestamp\": %u}}", red_raw, ir_raw, beat_per_minute, spo2, originalTime, timestamp - originCpuTime);
      i = 0;
    }
    else Data[i++] =  character;
  }
  client.loop();
}

void redrawReceivedInfo(int32_t ir_raw, int32_t red_raw, uint8_t bpm, uint8_t spo2) {
  tft.fillRoundRect(39, 116, 88, 18, 10, ST7735_BLACK);  // Draw rounded rectangle (x,y,width,height,radius,color)
  // It draws from the location to down-right
  tft.setCursor(43, 122);  // Set position (x,y)
  tft.setTextColor(ST7735_WHITE);  // Set color of text. First is the color of text and after is color of background
  tft.setTextSize(1);  // Set text size. Goes from 0 (the smallest) to 20 (very big)
  tft.println(ir_raw);
  tft.fillRoundRect(39, 141, 88, 18, 10, ST7735_BLACK);  // Draw rounded rectangle (x,y,width,height,radius,color)
  // It draws from the location to down-right
  tft.setCursor(43, 147);  // Set position (x,y)
  tft.setTextColor(ST7735_WHITE);  // Set color of text. First is the color of text and after is color of background
  tft.setTextSize(1);  // Set text size. Goes from 0 (the smallest) to 20 (very big)
  tft.println(red_raw); 
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
  tft.fillRect(0, 0, 128, 10, ST7735_BLACK);
  tft.setCursor(0, 0); // display will be halfway down screen
  tft.setTextColor(ST7735_WHITE);  // Set color of text. First is the color of text and after is color of background
  tft.setTextSize(1);
  tft.println(t);

  tft.setTextWrap(false); // Don't wrap text to next line

  if (offset < text.length() - 1) {
    offset++;
  } else {
    offset = 0;
  }
}

void drawGraph() {
  vectorNew.remove(0);
  if (ir_raw <= MOD_VALUE_SCREEN_PIXEL * HEIGHT_COORDINATE_AXIS + 1 && ir_raw >= 0) {
    vectorNew.push_back(ir_raw / MOD_VALUE_SCREEN_PIXEL);
  } else if (ir_raw > MOD_VALUE_SCREEN_PIXEL * HEIGHT_COORDINATE_AXIS) {
    vectorNew.push_back(HEIGHT_COORDINATE_AXIS);
  } else {
    vectorNew.push_back(0);
  }
  //tft.fillRect(0, 10, 100, 101, ST7735_BLACK); // Draw filled rectangle (x,y,width,height,color)
  for (int draw = vectorNew.size() - 1; draw >= 1 ; draw--) {
    //if (!vectorNew[draw] && !vectorNew[draw - 1]) break;
    tft.drawLine(draw * TIME_UNIT, ORGIN_RAW - vectorOld[draw], (draw - 1)*TIME_UNIT, ORGIN_RAW - vectorOld[draw - 1], ST7735_BLACK);
    vectorOld[draw] = vectorNew[draw];
    tft.drawLine(draw * TIME_UNIT, ORGIN_RAW - vectorNew[draw], (draw - 1)*TIME_UNIT, ORGIN_RAW - vectorNew[draw - 1], ST7735_WHITE);
  }
  vectorOld[0] = vectorNew[0];
}
