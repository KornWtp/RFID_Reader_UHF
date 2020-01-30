/*
  RFID Reader UHF & Esp32_EVB & MQTT

  version 1 - 28 Dec  2019
  author : KornWtp

*/

#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ETH.h>

#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER 12

#include <HardwareSerial.h>

#define RX  16    //Serial Receive pin
#define TX  17    //Serial Transmit pin

HardwareSerial RS232Serial(Serial1);

int request = 0;
char *cstring;

int relay1 = 32;
int relay2 = 33;

int door_status_pin = 14;
int door_status = 0;

// CHANGE THESE SETTINGS FOR YOUR APPLICATION
const char* mqttServerIp = "128.199.104.122"; // IP address of the MQTT broker
const short mqttServerPort = 1883; // IP port of the MQTT broker
const char* mqttClientName = "ESP32";
const char* mqttUsername = NULL;
const char* mqttPassword = NULL;

// Initializations of network clients
WiFiClient espClient;
PubSubClient mqttClient(espClient);
static bool eth_connected = false;
uint64_t chipid;

//ESP32 Specific Ethernet
//Yes, I know it says wifi.  Trust me.
void WiFiEvent(WiFiEvent_t event)
{
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("esp32-ethernet");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

void callback(char* topic, uint8_t* payload, unsigned int length) {
  for (int i = 0; i < length; i++) {
    payload[length] = '\0';
    cstring = (char *) payload;
  }

  request = atof(cstring);
  Serial.println(request);
  Serial.println(topic);

  // This snippet sets all the setpoints at once for the whole arduino
  // if (strstr(topic, "setall") != NULL) {

  // }

  if (strstr(topic, "relays/1") != NULL) {
    request = atof(cstring);
    Serial.println("The request is...");
    Serial.println(request);
    if (request == 1) {
      Serial.println("Turning relay 1 on");
      digitalWrite(relay1, HIGH);
      mqttClient.publish("status/", "Door open");
      RFID_Reader();
      delay(1000);
      request = 0;
    }
    if (request == 0) {
      Serial.println("Turning relay 1 off");
      digitalWrite(relay1, LOW);
    }
  }
  //  if (strstr(topic, "relays/2") != NULL) {
  //    request = atof(cstring);
  //    Serial.println("The request is...");
  //    Serial.println(request);
  //    if (request == 1) {
  //      Serial.println("Turning relay 2 on");
  //      digitalWrite(relay2, HIGH);
  //      delay(1000);
  //      request = 0;
  //    }
  //    if (request == 0) {
  //      Serial.println("Turning relay 2 off");
  //      digitalWrite(relay2, LOW);
  //    }
  //  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect(mqttClientName)) {
      // if (mqttClient.connect(mqttClientName, mqttUsername, mqttPassword)) { // if credentials is nedded
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish("random/test", "hello world");
      // ... and resubscribe
      mqttClient.subscribe("relays/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void Door_status() {
  door_status = digitalRead(door_status_pin);
  if (door_status == 0) {
    mqttClient.publish("status/", "Door closed");
    Serial.println("Door closed");
  }
  else {
    mqttClient.publish("status/", "Door open");
    Serial.println("Door open");
  }
  delay(1000);
}

void RFID_Reader() {
  // inquiry frame
  RS232Serial.write(0xFA);
  RS232Serial.write(0x07);
  RS232Serial.write(0xFF);
  RS232Serial.write(0x00);
  RS232Serial.write(0x20);
  RS232Serial.write(0x30);
  RS232Serial.write(0x36);
  RS232Serial.write(0xD7);

  byte data_buf[30];
  String RFID_Read = "";
  RS232Serial.readBytes(data_buf, 30);
  int len = 30;
  char t[3];
  byte* payload = data_buf;
  while (len--) {
    uint8_t b = *(payload++);
    sprintf(t, "%02x", b);
    RFID_Read += t;
    Serial.println(t);
  }
  mqttClient.publish("RFID_Reader/", RFID_Read.c_str());
  RFID_Read = "FFFF";
  for (byte i = 0; i < 30; i++ ) {
    Serial.print(data_buf[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");

  // clear frame
  //  RS232Serial.write(0xFA);
  //  RS232Serial.write(0x07);
  //  RS232Serial.write(0xFF);
  //  RS232Serial.write(0x00);
  //  RS232Serial.write(0x20);
  //  RS232Serial.write(0x04);
  //  RS232Serial.write(0xE7);
  //  RS232Serial.write(0xA6);
}

void setup()
{
  Serial.begin(115200);

  RS232Serial.begin(38400, SERIAL_8E1, RX, TX);

  pinMode(door_status_pin, INPUT_PULLUP);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);

#define BUTTON_PRESSED()  (!digitalRead (34))

  mqttClient.setServer(mqttServerIp, mqttServerPort);
  mqttClient.setCallback(callback);

  chipid = ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).
  Serial.printf("ESP32 Chip ID = %04X", (uint16_t)(chipid >> 32)); //print High 2 bytes
  Serial.printf("%08X\n", (uint32_t)chipid); //print Low 4bytes.

  WiFi.onEvent(WiFiEvent);
  ETH.begin();
}

void loop()
{
  // check if ethernet is connected
  if (eth_connected) {
    // now take care of MQTT client...
    if (!mqttClient.connected()) {
      reconnect();
    } else {
      mqttClient.loop();
      //      Door_satus();
    }
  }
  //  delay(1000);
}
