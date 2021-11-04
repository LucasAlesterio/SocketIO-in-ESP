uint8_t ledPin = D4;
const int servoPin = 14; //D5
const int tempPin = 4; //D2
const int buttonPin = 12; //D6;
const int Rpin = 5; //D1;
const int Gpin = 0; //D3;
const int Bpin = 13; //D7;
#define _WEBSOCKETS_LOGLEVEL_     3
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoJson.h>
#include <WebSocketsClient_Generic.h>
#include <SocketIOclient_Generic.h>
#include <Hash.h>
#include <DHT.h>
#define DHTTYPE DHT11 // DHT 11
#include <Servo.h>
#include <neotimer.h>
Servo servo;
ESP8266WiFiMulti WiFiMulti;
SocketIOclient socketIO;
IPAddress serverIP(128, 128, 128, 128);
uint16_t  serverPort = 3000;
DHT dht(tempPin, DHTTYPE);
float temperatura;
float umidade;
StaticJsonDocument <256> doc;
String output;
Neotimer mytimer = Neotimer(150);
void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
  doc.clear();
  output = "";

  switch (type) {
    case sIOtype_DISCONNECT:
      Serial.println("[IOc] Disconnected");
      break;

    case sIOtype_CONNECT:
      {
        Serial.println((char*) payload);
        // join default namespace (no auto join in Socket.IO V3)
        socketIO.send(sIOtype_CONNECT, "/");
        delay(200);
        JsonArray array = doc.to<JsonArray>();
        array.add("create");
        array.add("sala1");
        serializeJson(doc, output);
        socketIO.sendEVENT(output);
        Serial.println(output);
      }
      break;

    case sIOtype_EVENT:
      {
        Serial.println((char*) payload);
        // StaticJsonDocument <256> doc;
        deserializeJson(doc, payload);
        String typeEvent =  String(doc[0]);
        if (typeEvent == "led") {
          if (boolean(doc[1]["status"]) == false) {
            digitalWrite(ledPin, HIGH);
            Serial.println("Led On");
          } else {
            digitalWrite(ledPin, LOW);
            Serial.println("Led Off");
          }
        } else if (typeEvent == "servo") {
          int angle = String(doc[1]["angle"]).toInt();
          Serial.print("Servo to: ");
          Serial.println(angle);
          servo.write(angle);
        } else if (typeEvent == "tempRequest") {
          temperatura = dht.readTemperature();  //Realiza a leitura da temperatura
          umidade = dht.readHumidity(); //Realiza a leitura da umidade
          Serial.print("Temperatura: ");
          Serial.print(temperatura); //Imprime no monitor serial o valor da temperatura lida
          Serial.println(" ºC");
          Serial.print("Umidade: ");
          Serial.print(umidade); //Imprime no monitor serial o valor da umidade lida
          Serial.println(" %");
          JsonArray array = doc.to<JsonArray>();
          array.add("temp");
          JsonObject param1 = array.createNestedObject();
          param1["temperature"] = temperatura;
          param1["humidity"] = umidade;
          serializeJson(doc, output);
          socketIO.sendEVENT(output);
          Serial.println(output);
        }  else if (typeEvent == "ledColor") {
            Serial.println("Fita");
            analogWrite(Rpin,int(doc[1]["R"]));
            analogWrite(Gpin,int(doc[1]["G"]));
            analogWrite(Bpin,int(doc[1]["B"]));
        }
      }
      break;

    case sIOtype_ACK:
      Serial.print("[IOc] Get ack: ");
      Serial.println(length);
      hexdump(payload, length);
      break;

    case sIOtype_ERROR:
      Serial.print("[IOc] Get error: ");
      Serial.println(length);
      hexdump(payload, length);
      break;

    case sIOtype_BINARY_EVENT:
      Serial.print("[IOc] Get binary: ");
      Serial.println(length);
      hexdump(payload, length);
      break;

    case sIOtype_BINARY_ACK:
      Serial.print("[IOc] Get binary ack: ");
      Serial.println(length);
      hexdump(payload, length);
      break;

    default:
      Serial.print("Vixi ");
      break;
  }
}

void setup() {
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(Rpin, OUTPUT);
  pinMode(Gpin, OUTPUT);
  pinMode(Bpin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  servo.attach(servoPin);
  servo.write(0);
  dht.begin();
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\nStart ESP8266_WebSocketClientSocketIO on " + String(ARDUINO_BOARD));
  Serial.println(WEBSOCKETS_GENERIC_VERSION);
  if (WiFi.getMode() & WIFI_AP)
  {
    WiFi.softAPdisconnect(true);
  }
  WiFiMulti.addAP("name", "password");
  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();
  Serial.print("WebSockets Client started @ IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Connecting to WebSockets Server @ IP address: ");
  //  Serial.print(serverIP);
  Serial.print(serverIP);
  Serial.print(", port: ");
  Serial.println(serverPort);
  socketIO.setReconnectInterval(2000);
  socketIO.begin(serverIP, serverPort);
  // event handler
  socketIO.onEvent(socketIOEvent);
}

void loop()
{
  socketIO.loop();
  if (mytimer.repeat()) {
    int buttonState = digitalRead(buttonPin);
    if (buttonState == HIGH) {
      mytimer.repeatReset();
      doc.clear();
      output = "";
      Serial.println("Botão pressionado!");
      JsonArray array = doc.to<JsonArray>();
      array.add("button");
      JsonObject param1 = array.createNestedObject();
      param1["state"] = true;
      serializeJson(doc, output);
      socketIO.sendEVENT(output);
    }
  }

}
