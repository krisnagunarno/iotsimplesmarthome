#include <ESP32Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <MQTT.h>

const int RST_PIN = 22; // Reset pin
const int SS_PIN = 21; // Slave select pin

String Status;
String StatusPintu;

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance


Servo myservo; 
const int servoPin = 15;
const int kunciPin = 26;
const int ledTutup = 33;
const int ledBuka = 32;
const int doorSensor = 13;


const char ssid[] = "Krisna’s iPhone";
const char pass[] = "iphone123";

WiFiClient net;
MQTTClient client;

unsigned long lastMillis = 0;

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("arduino", "try", "try")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  client.subscribe("/hello");
  // client.unsubscribe("/hello");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  if (topic == "kunci" && payload == "1"){
    digitalWrite(kunciPin, HIGH);
  }
  else if (topic == "kunci" && payload == "0"){
    digitalWrite(kunciPin, LOW);
  }

  if (topic == "saklar" && payload == "1"){
    myservo.write(40);
    delay(1000);
  }
  else if (topic == "saklar" && payload == "0"){
    myservo.write(0);
    delay(1000);
  }
}

void setup() {

  WiFi.begin(ssid, pass);
 
	myservo.attach(servoPin); // attaches the servo on pin 18 to the servo object
  pinMode(kunciPin, OUTPUT);
  pinMode(ledTutup, OUTPUT);
  pinMode(ledBuka, OUTPUT);
  pinMode(doorSensor, INPUT_PULLUP);

  
  Serial.begin(115200);
  SPI.begin(); // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial(); // Show details of PCD - MFRC522 Card Reader details
  
  Status = "Nonaktif";

  myservo.write(0);    // tell servo to go to position in variable 'pos'
  digitalWrite(kunciPin, LOW);
  digitalWrite(ledTutup, LOW);
  digitalWrite(ledBuka, HIGH);

  client.begin("172.20.10.2", net);
  client.onMessage(messageReceived);
  connect();

  client.subscribe("kunci");
  client.subscribe("saklar");

  client.publish("saklar", "0");
  client.publish("kunci", "0");
}

void loop() {

  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    connect();
  }

  if (millis() - lastMillis > 1000) {
    lastMillis = millis();
    client.publish("/hello", "world");
    digitalRead(doorSensor);

    if(digitalRead(doorSensor) == HIGH) StatusPintu = "Open";
    else StatusPintu = "Closed";
      
    Serial.println(StatusPintu);

    
  }

      mfrc522.PCD_Init(); 
  
      //BAGIAN RFID
      // Look for new cards
      if ( ! mfrc522.PICC_IsNewCardPresent()) {
        return;
      }
       
      // Select one of the cards
      if ( ! mfrc522.PICC_ReadCardSerial()) {
        return;
      }
  
      String uid;
      String temp;
      for(int i=0;i<4;i++){
        if(mfrc522.uid.uidByte[i]<0x10){
          temp = "0" + String(mfrc522.uid.uidByte[i],HEX);
        }
        else temp = String(mfrc522.uid.uidByte[i],HEX);
        
        if(i==3){
          uid =  uid + temp;
        }
        else uid =  uid + temp+ " ";
      }
      Serial.println("UID "+uid);
      String grantedAccess = "09 C2 C3 7E"; //Akses via RFID yang ditunjuk
      grantedAccess.toLowerCase();

      if (uid == grantedAccess && (Status == "Aktif" && StatusPintu == "Closed")) {
        myservo.write(0);    
        delay(1000);
        digitalWrite(kunciPin, LOW);
        digitalWrite(ledTutup, LOW);
        digitalWrite(ledBuka, HIGH);
        client.publish("saklar", "0");
        client.publish("kunci", "0");
        Status = "Nonaktif";
        delay(1000);  
      }
      
      else if(uid == grantedAccess && (Status == "Nonaktif" || StatusPintu == "Closed")){
        myservo.write(40);    
        delay(1000);
        digitalWrite(kunciPin, HIGH);
        digitalWrite(ledTutup, HIGH);
        digitalWrite(ledBuka, LOW);
        client.publish("saklar", "1");
        client.publish("kunci", "1");
        Status = "Aktif";
        delay(5000);
        digitalWrite(kunciPin,LOW);
        client.publish("kunci", "0");
      }
      
      Serial.println("\n");
      mfrc522.PICC_HaltA();

}
