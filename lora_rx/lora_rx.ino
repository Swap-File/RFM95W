// Arduino9x_RX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (receiver)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Arduino9x_TX

#include <SPI.h>
#include <RH_RF95.h>

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Blinky on receipt
#define LED 13

boolean input_pin = false;
boolean input_pin_last = false;

#define EVENTS_TO_SAVE 20
uint32_t event_times[EVENTS_TO_SAVE];
uint8_t event_statuses[EVENTS_TO_SAVE];

#define EVENT_TURN_OFF 0  //turn off
#define EVENT_TURN_ON 1  //turn on
#define EVENT_POWER_UP 3  //power up

void setup()
{

  for (int i = 0; i < EVENTS_TO_SAVE; i++) {
    event_times[i] = 0;
    event_statuses[i] = EVENT_POWER_UP;
  }



  pinMode(6, INPUT_PULLUP);
  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);

  pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  while (!Serial);
  Serial.begin(9600);
  delay(100);

  Serial.println("Arduino LoRa RX Test!");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  rf95.setModemConfig(RH_RF95::Bw31_25Cr48Sf512);
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);




  input_pin = digitalRead(6);
  input_pin_last = input_pin;
  add_event();

}
void add_event() {
  for (int i = EVENTS_TO_SAVE - 1; i > 0; i--) {
    event_times[i] =  event_times[i - 1];
    event_statuses[i] = event_statuses[i - 1];
  }
  if (input_pin) {
    event_statuses[0] = EVENT_TURN_ON;
  } else {
    event_statuses[0] = EVENT_TURN_OFF;
  }
  event_times[0] = millis();

}

uint32_t last_time = 0;

void loop()
{

  input_pin = digitalRead(6);
  if ( input_pin_last != input_pin &&  millis() - last_time > 5000) {
    add_event();
    input_pin_last = input_pin;
    last_time = millis();
  }


  //Serial.println(digitalRead(6));
  if (rf95.available())  {
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len))
    {
      digitalWrite(LED, HIGH);
      RH_RF95::printBuffer("Received: ", buf, len);
      Serial.print("Got: ");
      Serial.println(buf[0]);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);

      uint8_t reply[6];

      for (int i; i < 6; i++) reply[i] = 0;

      int request = buf[0];


      if (request >= 0 && request < EVENTS_TO_SAVE) {
        Serial.println("good");
        uint32_t timdiff = 0;

        if (event_times[request] != 0) timdiff = millis() - event_times[request];

        reply[0] = (timdiff >> 0) & 0xFF;
        reply[1] = (timdiff >> 8) & 0xFF;
        reply[2] = (timdiff >> 16) & 0xFF;
        reply[3] = (timdiff >> 24) & 0xFF;
        reply[4] = event_statuses[request];
        reply[5] = buf[0];
      }

      rf95.send(reply, sizeof(reply));

      rf95.waitPacketSent();
      Serial.println("Sent a reply");
      digitalWrite(LED, LOW);
    }
    else
    {
      Serial.println("Receive failed");
    }
  }

}
