#include "SoftwareSerial.h"
#include <Ultrasonic.h>
#include <dht.h>
#include "Wire.h" 
#include "LiquidCrystal_I2C.h"
#define dht_dpin 2
#define DHT22_PIN 5
dht DHT;
String ssid = "dlink gg";
String password = "1991800218";
SoftwareSerial esp(8,10);// ESP_RX >> 4, ESP_TX >> 2
String data;
String server = "184.106.153.149"; // www.example.comXX change thingspeak ip
//String uri = "/arduino/add.php";// our example is /esppost.php
int t = 0;  // TEMPERATURE VAR
float h = 0;  // HUMIDITY VAR
int AnalogPin = A1;
float Avalue;
String GET = "GET /update?key=4EZXPLEJGM17J8SD";
//pm2.5
float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;
float AirQuality  = 0;
int ledPower = 2;
//line
String GETIF ="GET /trigger/notice/with/key/d7o4plBuFczA1sIjknIA_b";//curl -X POST https://maker.ifttt.com/
String serverif = "maker.ifttt.com";
int count=0;
#define TRIGGER_PIN  11
#define ECHO_PIN     12
Ultrasonic ultrasonic(TRIGGER_PIN, ECHO_PIN);
// LCD 
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
int f1,f2,f3,f4;

void setup() {
  esp.begin(115200);
  Serial.begin(115200);
  reset();  
  connectWifi();
  pinMode(ledPower,OUTPUT);
  lcd.begin(16, 2);
  lcd.backlight(); //LCD turn on light
}
void loop () {
  // data collecting
 //DHT
  DHT.read22(DHT22_PIN);
  h = DHT.humidity;
  t = DHT.temperature;
   //pm2.5
    digitalWrite(ledPower,LOW);
  delayMicroseconds(280);//samplingTime
  voMeasured = analogRead(0); // read the dust value
  delayMicroseconds(40);
   digitalWrite(ledPower,HIGH);
  delayMicroseconds(9680);
  calcVoltage =voMeasured * (5.0 / 1024.0);
  dustDensity = (0.17 * calcVoltage) - 0.1;
  AirQuality  =dustDensity*1000 ; //convert usually unit ug/m3  (float(dustVal/1024)-0.0356)*120000*0.035
  Serial.print("PM2.5(U g/m^3)=");// print
  Serial.print(AirQuality);
  delay(1000);
   Serial.print("\n");  
  //mq135
  Avalue = analogRead(AnalogPin);
  //Serial.print("AD=");// origin
  //Serial.print(Avalue);
   //Serial.print("\n");  
  Avalue=Avalue*9.87;
  if(Avalue>=1000){
     Avalue=999;
   }
  Serial.print("AD=");
  Serial.print(Avalue);            //send the data to the UART
  Serial.print("\n");  
  f1=(int)Avalue;
  f2=(int)t;
  f3=(int)h;
  f4=(int)AirQuality;
  
  data = "&field1=";
  data.concat(f1);  //Avalue
  data.concat("&field2=");
  data.concat(f2); //t
  data.concat("&field3=");
  data.concat(f3);//h
  data.concat("&field4=");
  data.concat(f4);//AirQuality
 
  //
  httppost();// update thingspeak database
  delay(1000);
  //LCD 
  lcd.setCursor(0, 0); 
  lcd.print("Temp: ");
  lcd.setCursor(5, 0);
  lcd.print(f2);
  lcd.setCursor(7, 0);
  lcd.print("oC");
  lcd.setCursor(9, 0);
  lcd.print("Hum: ");
  lcd.setCursor(13, 0);
  lcd.print(f3);
  lcd.setCursor(15, 0);
  lcd.print("%");
  lcd.setCursor(0, 1); 
  lcd.print("Air: ");
  lcd.setCursor(5, 1); 
  lcd.print(f1);
  
  // line trigger condition
  float cmMsec;
  boolean postline=false;
  long microsec = ultrasonic.timing();
  cmMsec = ultrasonic.convert(microsec, Ultrasonic::CM);
  Serial.print("MS: ");
  Serial.print(microsec);
  Serial.print(", CM: ");
  Serial.println(cmMsec);
  if(cmMsec>50.0){
    Serial.println("sent without paper ");
    postline=true;
    count++;
    }
   else{
    Serial.println("add paper");
     count=0;
     postline=false; 
     }
   if(count>2){
     Serial.println("still without  paper");
    postline=false;
    } 
    if(postline==true){
       Serial.println("sent line");
      httpline();// trigger IFTTT address
      }
  //delay(5000);
}
//reset the esp8266 module
void reset() {
  esp.println("AT+RST");
  //delay(1000);
  if (esp.find("OK") ) Serial.println("Module Reset");
}
void connectWifi() {
  String cmd = "AT+CWJAP=\"" + ssid + "\",\"" + password + "\"";
  esp.println(cmd);
 // delay(2000);//4000
  if (esp.find("OK")) {
    Serial.println("Connected!");
  }
  else {
    connectWifi();
    Serial.println("Cannot connect to wifi");
  }
}
// to thingspeak database
void httppost () {  
  esp.println("AT+CIPSTART=\"TCP\",\"" + server + "\",80");//start a TCP connection.
  if ( esp.find("OK")) {
    Serial.println("TCP connection ready");
  } delay(1000);
 /*String postRequest =
    "POST " + uri + " HTTP/1.0\r\n" +
    "Host: " + server + "\r\n" +
    "Accept: *" + "/" + "*\r\n" +
    "Content-Length: " + data.length() + "\r\n" +
    "Content-Type: application/x-www-form-urlencoded\r\n" +
    "\r\n" + data;*/
  String postRequest=GET +data+"\r\n";
  String sendCmd = "AT+CIPSEND=";//determine the number of caracters to be sent.
  esp.print(sendCmd);
  esp.println(postRequest.length() );
 // delay(3000);//500
  
if (esp.find(">")) {
    Serial.println("Sending.."); esp.print(postRequest);
    if ( esp.find("SEND OK")) {
      Serial.println("Packet sent");
      while (esp.available()) {
        String tmpResp = esp.readString();
        Serial.println(tmpResp);
      }
      // close the connection
      esp.println("AT+CIPCLOSE");
    }
  }
}


// line httppost  trigger IFTTT address
void httpline(){
   esp.println("AT+CIPSTART=\"TCP\",\"" + serverif + "\",80");//start a TCP connection.
  if ( esp.find("OK")) {
    Serial.println("TCP connection ready");
  } delay(1000);
 /*String postRequest =
    "POST " + uri + " HTTP/1.0\r\n" +
    "Host: " + server + "\r\n" +
    "Accept: *" + "/" + "*\r\n" +
    "Content-Length: " + data.length() + "\r\n" +
    "Content-Type: application/x-www-form-urlencoded\r\n" +
    "\r\n" + data;*/
  String postRequestifttt=GETIF+ " HTTP/1.1\r\n"+"Host: " + serverif + "\r\n" +  "Connection: close\r\n\r\n";
  String sendCmd = "AT+CIPSEND=";//determine the number of caracters to be sent.
  esp.print(sendCmd);
  esp.println(postRequestifttt.length() );
  //delay(3000);//500
  
if (esp.find(">")) {
    Serial.println("Sending.."); esp.print(postRequestifttt);
    if ( esp.find("SEND OK")) {
      Serial.println("Packet sent");
      while (esp.available()) {
        String tmpResp = esp.readString();
        Serial.println(tmpResp);
      }
      // close the connection
      esp.println("AT+CIPCLOSE");
    }
  }
}
