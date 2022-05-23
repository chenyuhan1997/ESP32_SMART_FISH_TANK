//网络库
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>





// 引入驱动OLED0.91所需的库
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


//舵机库
#include <Arduino.h>
#include <time.h>
#include <soc/rtc_cntl_reg.h>
#include <ESP32Servo.h>



#define OLED_RESET 14

#define ONE_WIRE_BUS 4


//DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>


//舵机初始建立项目
Servo myservo;
//DS18B20建立项目
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);












/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//cpu分配
#define pro_cpu   0   
#define app_cpu   1



//定义的是OLED的长和宽
#define SCREEN_WIDTH 128 // 设置OLED宽度,单位:像素
#define SCREEN_HEIGHT 32 // 设置OLED高度,单位:像素





//舵机部分引脚变量声明
const int servoPin = 12;
unsigned int Duoji_delay = 1000;
String DuojiValue = "1000";//起始值

//氧气泵部分声明
const int YangPin = 5;//氧气泵引脚
unsigned int Yang_delay = 1000;
String YangValue = "1000";


//DS18B20部分


String temperatureF = "";
String temperatureC = "";
unsigned long lastTime = 0;  
unsigned long timerDelay = 10000;//每十秒更新一次















/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);//ssd设置规范化

 




//网络声明
const  char  *ssid = "esp32"; //你的网络名称
const  char *password = "esp12345678"; //你的网络密码
const  char  *ntpServer = "pool.ntp.org"; //ntpserver的类型
const  long  gmtOffset_sec = 8 * 3600; //声明变量
const int daylightOffset_sec = 0;










//html输入文本
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//舵机
const char* PARAM_INPUT = "value";
//氧气泵
const char* PARAM_INPUT1 = "value1";


























/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
AsyncWebServer server(80);//将WEB服务器的端口pnning到80，就是服务器初始化的标准





















/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//定时器详情
static const uint16_t timer_divider = 80;//所有的定时器频率
static uint64_t alarm_timer_exact_count;//实际的计时器


















/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//这里的STATIC以及后面的函数，使用多任务进程
//任务处理的程序相关定义，也是声明一些内存任务的意思
static TaskHandle_t yang_task_handler;//主时钟任务
static TaskHandle_t duoji_task_handler;//普通时钟任务
static TaskHandle_t web_server_task_handler;//网页任务
static TaskHandle_t DS18B20_task_handler;//中断设置任务 
static TaskHandle_t oled_task_handler;//oled网页任务


















/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//定时器程序的相关定义，这里主要是声明的意思
static hw_timer_t* sync_timer_handler = NULL; //固定声明一个定时器









/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
// HTML web page界面的设置
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP SMART FISH-TANK</title>
  <style>
    html {font-family: Arial; display: inline-block; margin: 0px auto; text-align: center;}
    h2 {font-size: 2.3rem;}    
    p {font-size: 1.9rem;}
    body {max-width: 400px; margin:0px auto; padding-bottom: 25px;}
    .slider { -webkit-appearance: none; margin: 14px; width: 360px; height: 25px; background: #FFD65C;
      outline: none; -webkit-transition: .2s; transition: opacity .2s;}
    .slider::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 35px; height: 35px; background: #003249; cursor: pointer;}
    .slider::-moz-range-thumb { width: 35px; height: 35px; background: #003249; cursor: pointer; } 
   
    .units { font-size: 1.2rem; }
    .ds-labels{ font-size: 1.5rem; vertical-align:middle; padding-bottom: 15px; }   
  </style>

</head>
<body>
  <h2>ESP SMART FISH-TANK</h2>
 
  
  
  
  <h3>DUOJI PART</h3>
  <p><span id="textDuojiValue">%DUOJIVALUE%</span></p>
  <p><input type="range" onchange="updateDuojiPWM(this)" id="pwmDuoji" min="1000" max="10000" value="%DUOJIVALUE%" step="1000" class="slider"></p>
<script>
function updateDuojiPWM(element) {
  var DuojiValue = document.getElementById("pwmDuoji").value;
  document.getElementById("textDuojiValue").innerHTML = DuojiValue;
  console.log(DuojiValue);
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/Duoji?value="+DuojiValue, true);
  xhr.send();
}
</script>



  <h3>YANGQI PART</h3>
  <p><span id="textYangValue">%YANGVALUE%</span></p>
  <p><input type="range" onchange="updateYangPWM(this)" id="pwmYang" min="1000" max="10000" value1="%YANGVALUE%" step="1000" class="slider"></p>
<script>
function updateYangPWM(element) {
  var YangValue = document.getElementById("pwmYang").value;
  document.getElementById("textYangValue").innerHTML = YangValue;
  console.log(YangValue);
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/Yang?value1="+YangValue, true);
  xhr.send();
}
</script>



  <h3>ESP DS18B20 PART</h3>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="ds-labels">Temperature Celsius</span> 
    <span id="temperaturec">%TEMPERATUREC%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="ds-labels">Temperature Fahrenheit</span>
    <span id="temperaturef">%TEMPERATUREF%</span>
    <sup class="units">&deg;F</sup>
  </p>


  
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperaturec").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperaturec", true);
  xhttp.send();
}, 10000) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperaturef").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperaturef", true);
  xhttp.send();
}, 10000) ;
</script>


<h3>IF TEMPERATUREF less than 25 , JIARE WILL WORK, bigger than 30 , STOP WORK </h3>



</body>
</html>
)rawliteral";



//字符读取
String processor(const String& var){
  //Serial.println(var);
  if(var == "DUOJIVALUE"){
    return DuojiValue;
  }
  else if(var == "YANGVALUE"){
    return YangValue;
    }
  else if(var == "TEMPERATUREC"){
    return temperatureC;
  }
  else if(var == "TEMPERATUREF"){
    return temperatureF;
  }
    
  return String();
}
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//DS18B20 STRING
String readDSTemperatureC() {
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures(); 
  float tempC = sensors.getTempCByIndex(0);

  if(tempC == -127.00) {
    Serial.println("Failed to read from DS18B20 sensor");
    return "--";
  } else {
    Serial.print("Temperature Celsius: ");
    Serial.println(tempC); 
  }
  return String(tempC);
}



String readDSTemperatureF() {
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures(); 
  float tempF = sensors.getTempFByIndex(0);

  if(int(tempF) == -196){
    Serial.println("Failed to read from DS18B20 sensor");
    return "--";
  } else {
    Serial.print("Temperature Fahrenheit: ");
    Serial.println(tempF);
  }
  return String(tempF);
}
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//实时时间获取
//实时时间获取
//实时时间获取
void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    display.println("Failed to obtain time");
    return;
  }
  display.println(&timeinfo, "%F"); // 格式化输出
  display.println(&timeinfo, "%T"); // 格式化输出
  display.println(&timeinfo, "%A"); // 格式化输出
}

void OLED_Function(void *pvParameters){
  for(;;){
  vTaskDelay(1000);
  //清除屏幕
  display.clearDisplay();
  //设置光标位置
  display.setCursor(0, 0);
  printLocalTime();
  display.display();
    }
  }










/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//网页服务
void web_server_task_function(void* paramters)
{
  Serial.println("Setting up the HTTP server");//串口打印设置HTTP服务器
 // Send web page with input fields to client
   // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });



  // Send a GET request to <ESP_IP>/slider?value=<inputMessage>
  server.on("/Duoji", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      DuojiValue = inputMessage;
      Duoji_delay = DuojiValue.toInt();
      Serial.println("duoji change");
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });


  server.on("/Yang", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage1;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage1>
    if (request->hasParam(PARAM_INPUT1)) {
      inputMessage1 = request->getParam(PARAM_INPUT1)->value();
      YangValue = inputMessage1;
      Yang_delay = YangValue.toInt();
      Serial.println("yang qi change");
    }
    else {
      inputMessage1 = "No message sent";
    }
    Serial.println(inputMessage1);
    request->send(200, "text/plain", "OK");
  });

  server.on("/temperaturec", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", temperatureC.c_str());
  });

  server.on("/temperaturef", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", temperatureF.c_str());
  });
  // Start server
  server.begin();
  Serial.println("HTTP server setup completed");
  vTaskDelete(NULL);
  
}


/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

void DuojiWeishi(){
  myservo.write(0); 
  myservo.write(90);
  vTaskDelay(500/portTICK_PERIOD_MS);
  myservo.write(0);
  vTaskDelay(50/portTICK_PERIOD_MS);
  }

void Duoji_Function(void *pvParameters){
  while(1){
  DuojiWeishi();
  Serial.println("duoji work!");
//  delay(Duoji_delay);
  vTaskDelay(Duoji_delay/portTICK_PERIOD_MS);
  }
    }


/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

void yangqigongzuoyici(){
  
  digitalWrite(YangPin,LOW);
  //延迟1秒钟
  vTaskDelay(2000/portTICK_PERIOD_MS);
  //向io 口写入底电平
  digitalWrite(YangPin,HIGH);
  //延迟1秒钟
  vTaskDelay(2000/portTICK_PERIOD_MS);
  
  }

void yang_task_function(void *pvParameters){
  while(1){
    yangqigongzuoyici();
    //delay(Yang_delay);
    Serial.println("yangqi work!");
    vTaskDelay(Yang_delay/portTICK_PERIOD_MS);
  }
}




/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
void DS18B20_function(void *pvParameters){
  while(1){
    if ((millis() - lastTime) > timerDelay) {
    temperatureC = readDSTemperatureC();
    temperatureF = readDSTemperatureF();
    lastTime = millis(); 
    }
    Serial.println("DS18B20 START");
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/





void setup() {
  //串口监视器
  Serial.begin(115200);
    while(!Serial) 
  {
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
  vTaskDelay(100/portTICK_PERIOD_MS);
  Serial.println("executing the setup task..");
  Serial.println("('_')7");
  sensors.begin();
  
  temperatureC = readDSTemperatureC();
  temperatureF = readDSTemperatureF();
  //设置I2C的两个引脚SDA和SCL，这里用到的是17作为SDA，16作为SCL
  Wire.setPins(/*SDA*/21,/*SCL*/22);
  //初始化OLED并设置其IIC地址为 0x3C
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  //清除屏幕
  display.clearDisplay();
  //设置字体颜色,白色可见
  display.setTextColor(WHITE);
  //设置字体大小
  display.setTextSize(1.6);
  //设置光标位置
  display.setCursor(0, 0);
  //wifi设置
  Serial.printf("Connecting to %s ", ssid);


  
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    vTaskDelay(250/portTICK_PERIOD_MS);
  }
  Serial.println();
  Serial.println("Connected to WiFi");
  Serial.printf("WiFi: %s\n", ssid); 
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  WiFi.mode(WIFI_STA);
  myservo.setPeriodHertz(70); 
  myservo.attach(servoPin);


  
 //GPIO初始化
  pinMode(servoPin, OUTPUT);
  pinMode(YangPin,OUTPUT);





  
//  pinMode(dismiss_button, INPUT_PULLUP);
//  pinMode(Yuyin_IO, OUTPUT);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);//关闭低电压检测,避免无限重启
         
  //中断源只想cpu0 
  xTaskCreatePinnedToCore(yang_task_function, "yang_task", 2048, NULL, 1, &yang_task_handler, app_cpu);//yang task
  xTaskCreatePinnedToCore(Duoji_Function, "duoji_task", 2048, NULL, 1, &duoji_task_handler, app_cpu);//alarm task
  xTaskCreatePinnedToCore(web_server_task_function, "server_task", 6000, NULL, 1, &web_server_task_handler, pro_cpu);//web server tasks
  xTaskCreatePinnedToCore(OLED_Function, "oled_task", 2048, NULL, 1, &oled_task_handler, app_cpu);
  xTaskCreatePinnedToCore(DS18B20_function, "DS18B20_task", 2048, NULL, 1, &DS18B20_task_handler, app_cpu);

  //NTPClient初始化
  vTaskDelay(100/portTICK_PERIOD_MS);
  vTaskDelete(NULL);//删除主要任务    
  
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);//关闭低电压检测,避免无限重启

  
}


void loop() {

}
