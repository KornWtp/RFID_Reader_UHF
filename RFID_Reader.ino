/*
  RFID Reader UHF

  version 1 - 3 Dec  2019
  author : KornWtp

*/

#include <HardwareSerial.h>

#define RX  16    //Serial Receive pin
#define TX  17    //Serial Transmit pin

HardwareSerial RS232Serial(Serial1);

void setup() {

  Serial.begin(115200);
  Serial.println("RFID Reader UHF");

  RS232Serial.begin(38400, SERIAL_8E1, RX, TX);
}

void loop() {
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
  RS232Serial.readBytes(data_buf, 30);

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


  delay(2000);
}
