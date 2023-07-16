#include "EEPROM.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <DHT.h>

//header file
#include "esp_pin.h"
#include "camera_config.h"


//define pin
#define pinDHT 15
DHT dht(pinDHT, DHT22);
#define microwaveSensor 13
#define flameSensor 14
#define TrigPin 12
#define EchoPin 2
#define FLASH_LED_PIN 4
bool flashState = LOW;

//vaiabel
int motion, flame, distance;
float kelembaban, suhu;
bool isFlame = false, isMotion = false, isDistance = false, isTemp = false, isFire = false;
long duration;

//eeprom
int setDistance; //read from eeprom 1
int addresDistance = 1;
int addressFlame = 3;
int addressMotion = 2;


//alarm
bool detectDistance = false, detectFlame = true, detectMotion = true;


//WIFI

const char* ssid = "YOUR SSID";
const char* password = "YOUR WIFI Password";

//Telegram BOT
String BOTtoken = "YOUR:BOT TOKEN";  // your Bot Token (Get from Botfather)
String CHAT_ID = "YOUR CHATID";     // your Chat id (Get from Idbot)
bool sendPhoto = false;
WiFiClientSecure clientTCP;
UniversalTelegramBot bot(BOTtoken, clientTCP);


//Schedule 
unsigned long scheduleTIme = millis();
int microwaveFlag = 0;
int ultrasonicFlag=0;
int flameFlag=0;
int tempFlag=0;

//Checks for new messages Telegram every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;


//function Sensor

void detectFire(){
  if(isTemp == true and isFlame == true){
      if(isFire == false){
        bot.sendMessage(CHAT_ID, "Terindikasi Kebakaran, Terdapat suhu tinggi dan terdeteksi api", "");
        isFire = true;
      }
    }else{
      isFire= false;
    }
}


void readTempHumidity(){
  kelembaban = dht.readHumidity();
  suhu = dht.readTemperature();
  Serial.print("Suhu dan Kelembaban : ");
  Serial.print(suhu);
  Serial.print(", ");
  Serial.print(kelembaban);
  Serial.println("");

  if(detectFlame == true){
   if(suhu > 40.00){
      if(isTemp == false){
        bot.sendMessage(CHAT_ID, "Terdeteksi Suhu Tinggi disekitar Pratima", "");
        isTemp = true;
      }
     }else{
        isTemp = false;
     }
  }
}

void readUltrasonic(){
  digitalWrite(TrigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(TrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(TrigPin, LOW);
  duration = pulseIn(EchoPin, HIGH);
  distance = duration * 0.034 / 2;
  Serial.println("Deteksi jarak : ");
  Serial.println(detectDistance ? "Ya" : "Tidak");
  Serial.print("Jarak : ");
  Serial.print(distance);
  Serial.println("");
  if(detectDistance == true){
    if(setDistance < distance-1 || setDistance > distance+1){
      if(isDistance == false){
        bot.sendMessage(CHAT_ID, "Pratima Tidak pada tempatnya", "");
        isDistance = true;
      }
    }else{
      isDistance = false;
    }
  }
}

void readFlame(){
  flame = digitalRead(flameSensor);
  Serial.print("Deteksi Api: ");
  Serial.println(flame);
  Serial.print("Detect flame : ");
  Serial.println(detectFlame);
  Serial.print(flame ? "Tidak Ada Api" :"Ada Api");
  Serial.println("");

  if(detectFlame == true){
    if(flame == 0){
      if(isFlame == false){
         bot.sendMessage(CHAT_ID, "Terdeteksi Api", "");
        isFlame = true; 
      }           
    }else{
      isFlame = false;
    }
  }
}

void readMotion(){
  motion = digitalRead(microwaveSensor);
  Serial.print("Motion: ");
  Serial.println(motion? "Bergerak": "Tidak");
  Serial.print("Detect motion : ");
  Serial.println(detectMotion);
  Serial.println(isMotion);
  if(detectMotion == true){
    if(motion == 1){
      if(isMotion == false){
        Serial.println("motion detect");
          bot.sendMessage(CHAT_ID, "Terdeteksi Gerakan", "");
          flashState = HIGH;
          digitalWrite(FLASH_LED_PIN, flashState);
          sendPhotoTelegram();
//          delay(500);
//          flashState = LOW;
//          digitalWrite(FLASH_LED_PIN, flashState);
        isMotion = true;
      }
    }else{
      isMotion = false;
    }
  }
}

//function telegram
void handleNewMessages(int numNewMessages) {
  Serial.print("Handle New Messages: ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);
    String from_name = bot.messages[i].from_name;
    if (text == "/start"){
      String welcome = "Welcome , " + from_name + "\n";
      welcome += "Berikut ini merupakan perintah/Command yang ada pada Rancang Sistem Monitoring Pratima Berbasis IoT \n";
      welcome += "/photo : Mengambil foto keadaan tempat penyimpanan pratima \n";
      welcome += "/status_alarm : Melihat status fitur monitoring pratima \n";
      welcome += "/suhu : Menampilkan data Suhu dan Kelembaban \n";
      welcome += "/posisi : Mengecek jarak pratima dengan perangkat IoT \n";
      welcome += "/setjarak : Setting jarak monitoring pratima \n";
      welcome += "/resetjarak : Reset jarak monitoring pratima \n";
      welcome += "/off_deteksi_pergerakan : Mematikan fitur deteksi Pergerakan \n";
      welcome += "/off_deteksi_kebakaran : Mematikan fitur deteksi Kebakaran \n";
      welcome += "/on_deteksi_pergerakan : Menghidupkan fitur deteksi Pergerakan \n";
      welcome += "/on_deteksi_kebakaran : Menghidupkan fitur deteksi Kebakaran \n";
      bot.sendMessage(CHAT_ID, welcome, "");
    }else if (text == "/flash"){
      flashState = !flashState;
      digitalWrite(FLASH_LED_PIN, flashState);
      Serial.println("Change flash LED state");
    }else if (text == "/photo"){
      sendPhoto = true;
      Serial.println("New photo request");
    }else if (text == "/suhu"){
      String datasuhu = "Suhu sekarang: " + String(suhu) + "\nKelembaban sekarang: " + String(kelembaban) + "\n";
      bot.sendMessage(chat_id, datasuhu, "");
      Serial.println("Read Suhu dan kelembaban");
    }else if(text == "/posisi"){
      String statusMeasure = detectDistance ? "Disetting" : "Tidak Disetting" ;
      String dataJarak = "Pengukuran Jarak Pratima Saat ini " + statusMeasure +"\nJarak Pratima dengan alat sekitar : " + String(distance) + "CM,";
      bot.sendMessage(chat_id, dataJarak, "");
      Serial.println("Read data Jarak");
    }else if (text == "/setjarak"){
      EEPROM.write(addresDistance, distance);
      EEPROM.commit();
      detectDistance = true;
      setDistance = distance;
      String setjarak = "Jarak Pratima diatur sejauh " + String(distance) + "CM";
      bot.sendMessage(chat_id, setjarak, "");
      Serial.println("Set data Jarak");
    }else if(text == "/resetjarak"){
      EEPROM.write(addresDistance, 0);
      EEPROM.commit();
      detectDistance = false;
      setDistance = 0;
      bot.sendMessage(chat_id, "Seting Jarak pengukuran pratima dihapus menjadi 0", "");
      Serial.println("Reset Jarak");
    }else if(text == "/read"){
      int distanceEEprom = EEPROM.read(addresDistance);
      bool read2 = EEPROM.read(addressMotion);
      bool read3 = EEPROM.read(addressFlame);
      String kata = "Jarak eeprom = " + String(distanceEEprom) + "CM, Jarak Setting Sekarang: " + String(setDistance) +"CM, \nStatus pergerakan = " + String(read2) + "Status Kebakaran = " + String(read3);
      bot.sendMessage(chat_id, kata, "");
      Serial.println("Read EEPROM");
    }else if(text == "/off_deteksi_pergerakan"){
      detectMotion = false;
      EEPROM.write(addressMotion, false);
      EEPROM.commit();
      isMotion = false;
      bot.sendMessage(chat_id, "Deteksi Pergerakan Dimatikan", "");
      Serial.println("Off deteksi pergerakan");
    }else if(text == "/off_deteksi_kebakaran"){
      detectFlame = false;
      EEPROM.write(addressFlame, false);
      EEPROM.commit();
      isTemp = false;
      isFlame = false;
      bot.sendMessage(chat_id, "Deteksi Kebakaran Dimatikan", "");
      Serial.println("Off deteksi kebakaran");
    }else if(text == "/on_deteksi_pergerakan"){
      detectMotion = true;
      EEPROM.write(addressMotion, true);
      EEPROM.commit();
      isMotion = false;
      bot.sendMessage(chat_id, "Deteksi Pergerakan Dihidupkan", "");
      Serial.println("On deteksi pergerakan");
    }else if(text == "/on_deteksi_kebakaran"){
      detectFlame = true;
      EEPROM.write(addressFlame, true);
      EEPROM.commit();
      isFlame = false;
      isTemp = false;
      bot.sendMessage(chat_id, "Deteksi Kebakaran Dihidupkan", "");
      Serial.println("On deteksi kebakaran");
    }else if(text == "/status_alarm"){
      String kata = "Deketsi pergerakan = " + String(detectMotion) + ", Deteksi Kebakaran = " + String(detectFlame) + ", Deteksi Jarak Pratima = " + String(detectDistance);
      bot.sendMessage(chat_id, kata, "");
      Serial.println("Cek Status Deteksi Monitoring");
    }
    else{}
  }
}

//function Photo
String sendPhotoTelegram() {
  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }
  Serial.println("Connect to " + String(myDomain));

  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connection successful");
    
    String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + CHAT_ID + "\r\n--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--RandomNerdTutorials--\r\n";

    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;
  
    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
    clientTCP.println();
    clientTCP.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        clientTCP.write(fbBuf, remainder);
      }
    }  
    
    clientTCP.print(tail);
    
    esp_camera_fb_return(fb);
    
    int waitTime = 10000;   // timeout 10 seconds
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + waitTime) > millis()){
      Serial.print(".");
      delay(100);      
      while (clientTCP.available()) {
        char c = clientTCP.read();
        if (state==true) getBody += String(c);        
        if (c == '\n') {
          if (getAll.length()==0) state=true; 
          getAll = "";
        } 
        else if (c != '\r')
          getAll += String(c);
        startTimer = millis();
      }
      if (getBody.length()>0) break;
    }
    clientTCP.stop();
    Serial.println(getBody);
  }
  else {
    getBody="Connected to api.telegram.org failed.";
    Serial.println("Connected to api.telegram.org failed.");
  }
  if(flashState == HIGH){
      flashState=LOW;
      digitalWrite(FLASH_LED_PIN, flashState);
  }
  return getBody;
}

void setup(){
  // Init Serial Monitor
  Serial.begin(115200);
  EEPROM.begin(512);
  pinMode(microwaveSensor, INPUT);
  pinMode(flameSensor, INPUT);
  pinMode(TrigPin, OUTPUT);
  pinMode(EchoPin, INPUT);
  dht.begin();

  // Set LED Flash as output
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, flashState);

  // Config and init the camera
  configInitCamera();

  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP()); 
  
  //read EEPROM
  setDistance = EEPROM.read(addresDistance);
  Serial.print("Jarak = ");
  Serial.println(EEPROM.read(addresDistance));
  if(setDistance!= 0){
    detectDistance = true;
  }
  detectMotion = EEPROM.read(addressMotion);
  detectFlame = EEPROM.read(addressFlame);
}

void loop() {
  if (sendPhoto) {
    Serial.println("Preparing photo");
    flashState = HIGH;
    digitalWrite(FLASH_LED_PIN, flashState);
    sendPhotoTelegram(); 
    sendPhoto = false;
  }

  if(millis() >  scheduleTIme + 1000){
    microwaveFlag++;
    ultrasonicFlag++;
    flameFlag++;
    tempFlag++;

    if(microwaveFlag%1==0){
//      deteksi gerakan
      readMotion();
      microwaveFlag=0;
    }
    if(ultrasonicFlag%2==0){
//      deteksi jarak 
      readUltrasonic();
      ultrasonicFlag=0;
    }
    if(flameFlag%3==0){
//      deteksi api
      readFlame();
      flameFlag;
    }
    if(tempFlag%3==0){
//      deteksi suhu dan kelembaban
      readTempHumidity();
      tempFlag=0;
    }
    if(detectFlame == true){
      detectFire();
    }
    scheduleTIme = millis();
  }
  
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}
