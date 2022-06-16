#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <WiFiMulti.h>

// Uncomment the type of sensor in use
#define DHT_TYPE    DHT11     // DHT 11
//#define DHT_TYPE    DHT22     // DHT 22 (AM2302)
//#define DHT_TYPE    DHT21     // DHT 21 (AM2301)

/*像素灯设定*/
#define PIXEL_PER_SEGMENT  2     // Number of LEDs in each Segment
#define PIXEL_DIGITS       4     // Number of connected Digits
#define PIXEL_PIN          14     // GPIO Pin
#define PIXEL_DASH         1    // Binary segment

/*附加功能引脚设定*/
#define LDR_PIN       A0    // LDR pin
#define DHT_PIN       13    // DHT Sensor pin
#define BUTTON_PIN    12    // Button pin

/*时间格式*/
#define TIME_FORMAT        24    // 12 = 12 hours format || 24 = 24 hours format

/*创建类函数*/
Adafruit_NeoPixel strip = Adafruit_NeoPixel((PIXEL_PER_SEGMENT * 7 * PIXEL_DIGITS) + (PIXEL_DASH * 2),\
 PIXEL_PIN, NEO_GRB + NEO_KHZ800);
DHT dht(DHT_PIN, DHT_TYPE);//温度

// set Wi-Fi SSID and password
const char *ssid     = "BB2F";
const char *password = "bbf21338";

WiFiUDP ntpUDP;

WiFiMulti wifiMulti;
// 'time.nist.gov' is used (default server) with +1 hour offset (3600 seconds) 60 seconds (60000 milliseconds) update interval
NTPClient timeClient(ntpUDP, "time.nist.gov", 28800, 60000); //GMT+5:30 : 5*3600+30*60=19800//GMT+8 : 8*3600 = 28800

/*时间刷新频率*/
int period = 1000;   //Update frequency  = 1second
unsigned long time_now = 0;
int Second, Minute, Hour;

// set default brightness
int Brightness = 40;
int changePix = 0 ;
int pixBrig = 20;
// current temperature, updated in loop()
int Temperature;

bool Show_Temp = false;

//Digits array
byte digits[12] = {
  //abcdefg
  0b1111110,     // 0
  0b0110000,     // 1
  0b1101101,     // 2
  0b1111001,     // 3
  0b0110011,     // 4
  0b1011011,     // 5
  0b1011111,     // 6
  0b1110000,     // 7
  0b1111111,     // 8
  0b1110011,     // 9
  0b1001110,     // C
  0b1000111,     // F
};

//彩虹渐变效果

uint32_t Wheel(byte WheelPos){
  if(WheelPos < 85){
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if(WheelPos < 170){
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
   else{
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

void rainbow(uint8_t wait){
  uint16_t i, j;
  for(j=0; j<256; j++){
    for(i=0; i<strip.numPixels(); i++){
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Clear all the Pixels
void clearDisplay() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
}

//一键配网
void smartConfig(){

  WiFi.mode(WIFI_STA);

  Serial.println("\r\nWait for Smartconfig.");

  WiFi.beginSmartConfig();

  while (!WiFi.smartConfigDone()) {
    Serial.print(".");
    rainbow(2);
  }

  Serial.println("SmartConfig Success");

  Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());

  Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());

  wifiMulti.addAP(WiFi.SSID().c_str(),WiFi.psk().c_str());

}

void setup() {
  Serial.begin(38400);
  strip.begin();
  strip.setBrightness(pixBrig);
  clearDisplay();
  strip.show();

  dht.begin();
  pinMode(BUTTON_PIN, INPUT);

  WiFi.begin(ssid, password);

  if( WiFi.status() != WL_CONNECTED ){
    for(int i=0 ; i<=10 ; i++){
      delay(500);
      Serial.print(".");
    }   
    if(WiFi.status() == WL_CONNECTED ){
      Serial.println("Wifi connected");
      return;
    }
    Serial.println("connection failed");
    wifiMulti.addAP(ssid, password);
    wifiMulti.addAP("小米共享WiFi_A6CB", "669669669");
    wifiMulti.addAP("669", "669669669");
  // wifiMulti.addAP("BB2F", "bbf21338");
    if(wifiMulti.run() == WL_CONNECTED){
      // delay(1000);
      Serial.println("Multi connected");
    }
  }
  if(wifiMulti.run() != WL_CONNECTED){
      // delay(1000);
    Serial.println("Multi not connected");
    smartConfig();
  }
  // else if( WiFi.status() != WL_CONNECTED ) {
  //   // delay(500);
    
  //   // Serial.print("Connecting.");
  //   delay(2000); 
  //   Serial.println("airkiss.");
  //   smartConfig();
  // }
  Serial.println("connected");
  // NTP.begin ();
  timeClient.begin();
  delay(10);

}

void loop() {
  if (WiFi.status() == WL_CONNECTED) { // check WiFi connection status
    int sensor_val = analogRead(LDR_PIN);
    Brightness =40;
    timeClient.update();
    int Hours;
    unsigned long unix_epoch =  timeClient.getEpochTime();   // get UNIX Epoch time
    Second = second(unix_epoch);                            // get seconds
    Minute = minute(unix_epoch);                            // get minutes
    Hours  = hour(unix_epoch);                              // get hours

    if (TIME_FORMAT == 12) {//12小时制判断处理
      if (Hours > 12) {
        Hour = Hours - 12;
      }
      else
        Hour = Hours;
    }
    else
      Hour = Hours;
  }

  //时间和温度显示切换
  // if (digitalRead(BUTTON_PIN) == LOW) {
  //   Show_Temp = true;
  // }
  // else
    Show_Temp = false;

  if (Show_Temp) {
    Temperature = dht.readTemperature();
    Serial.println(Temperature);
    clearDisplay();
    writeDigit(0, Temperature / 10);
    writeDigit(1, Temperature % 10);
    writeDigit(2, 10);
    strip.setPixelColor(28, strip.Color(Brightness, Brightness,  Brightness));
    strip.show();
    delay(3000);
    clearDisplay();
    Show_Temp = false;
  }
  while (millis() > time_now + period) {
    time_now = millis();
    Serial.print(Hour);
    Serial.print(":");
    Serial.print(Minute);
    Serial.print(":");
    Serial.println(Second);
    disp_Time();     // Show Time
  }
 }
//显示时间
void disp_Time() {
  clearDisplay();
  writeDigit(0, Hour / 10);
  writeDigit(1, Hour % 10);
  writeDigit(2, Minute / 10);
  writeDigit(3, Minute % 10);
  writeDigit(4, Second / 10);
  writeDigit(5, Second % 10);
  disp_Dash();
  strip.show();
}
//显示冒号
void disp_Dash() {
  int dot, dash;
  for (int i = 0; i < 2; i++) {
    dot = 2 * (PIXEL_PER_SEGMENT * 7) + i;//找到代表冒号灯的位置
    for (int j = 0; j < PIXEL_DASH; j++) {//设置全部N颗冒号灯
      dash = dot + j * (2 * (PIXEL_PER_SEGMENT * 7) + 2);
      Second % 2 == 0 ? strip.setPixelColor(dash, strip.Color(0, 255, 255)) : strip.setPixelColor(dash, strip.Color(0, 0,0));
      //颜色相同，每两秒调用一次设置，闪烁
    }
  }
}

//写入数字
void writeDigit(int index, int val) {
  byte digit = digits[val];
  int margin;
  if (index == 0 || index == 1 ) margin = 0;//时
  if (index == 2 || index == 3 ) margin = 1;//分
  if (index == 4 || index == 5 ) margin = 2;//秒
  for (int i = 6; i >= 0; i--) {//7段，循环7次
    int offset = index * (PIXEL_PER_SEGMENT * 7) + i * PIXEL_PER_SEGMENT + margin * 2;
    //0*14+6*2+0*2 = 12
    uint32_t color;
    /*
    浅紫245, 207, 247
    淡蓝151, 241, 239
    蓝0, 25, 255||0, 51, 255
    */
    uint32_t qPurple = strip.Color(255, 92, 255);
    uint32_t dBlue = strip.Color(169, 255, 255);
    uint32_t mBlue = strip.Color(0, 255, 255);
    uint32_t tBlue = strip.Color(0, 25, 255);

    if (digit & 0x01 != 0) {//如果右末位数字为1，设置显示颜色，否则不显示
      //时分秒分别设置颜色
      // if (index == 0 || index == 1 ) {
      //   // if (i==0 || i==1 || i==5) {
      //     color = strip.Color(changePix, changePix,  Brightness);//蓝色
      // //   }else
      // // color = strip.Color(Brightness, 0,  Brightness);
      // }
      // if (index == 2 || index == 3 ) color = strip.Color(Brightness, 0,Brightness);//紫色
      // if (index == 4 || index == 5 ) color = strip.Color(Brightness, 0,  0);//红色

      switch(i){
        case 0 : color = qPurple; break;
        case 1 : color = dBlue; break;
        case 5 : color = dBlue; break;
        case 6 : color = tBlue; break;
        default : color = mBlue; break;
      }
    }
    else
      color = strip.Color(0, 0, 0);

    for (int j = offset; j < offset + PIXEL_PER_SEGMENT; j++) {//点亮每一段所有的灯
      strip.setPixelColor(j, color);
    }
    digit = digit >> 1;
  }
}

