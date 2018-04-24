// LoRa 9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example LoRa9x_RX

#include <SPI.h>
#include <RH_RF95.h>

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

void setup()
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  while (!Serial);
  Serial.begin(9600);
  delay(100);

  Serial.println("Arduino LoRa TX!");

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
}


void loop()
{
  Serial.println(" ");
  Serial.println("Pump Log Downloading...");
  // Send a message to rf95_server

  float min_rssi = 0;
  float max_rssi = 0;
  float average_rssi = 0;

  int packet_req = 0;
  int retries = 0;

  uint32_t result_mode[20];
  uint32_t result_time[20];


  while (packet_req < 20 && retries < 10) {

    char radiopacket[20];

    radiopacket[0] = packet_req;

    // Serial.println("Sending..."); delay(20);
    rf95.send((uint8_t *)radiopacket, 2);

    // Serial.println("Waiting for packet to complete..."); delay(20);
    rf95.waitPacketSent();
    // Now wait for a reply
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);



    // Serial.println("Waiting for reply..."); delay(50);
    if (rf95.waitAvailableTimeout(2000))
    {
      // Should be a reply message for us now
      if (rf95.recv(buf, &len))
      {
        /*
                Serial.print(buf[0]);
                Serial.print('\t');
                Serial.print(buf[1]);
                Serial.print('\t');
                Serial.print(buf[2]);
                Serial.print('\t');
                Serial.print(buf[3]);
                Serial.print('\t');
                Serial.print(buf[4]);
                Serial.print('\t');
                Serial.print(buf[5] - 48);
                Serial.println();
        */

        Serial.print("Log Entry: ");
        if ((int)buf[5] < 10) Serial.print("0");
        Serial.print((int)buf[5]);
        Serial.print(" ");
        result_mode[packet_req] = buf[4];
        print_mode(result_mode[packet_req],true);

        Serial.print(" Time: ");

        uint32_t replytime = buf[3];
        replytime = replytime << 8;
        replytime = replytime | buf[2];
        replytime = replytime << 8;
        replytime = replytime | buf[1];
        replytime = replytime << 8;
        replytime = replytime | buf[0];

        result_time[packet_req] = replytime;
        Serial.println(result_time[packet_req]);

        //print_time(replytime);

        if (packet_req == 0) {

          min_rssi = rf95.lastRssi();
          max_rssi = rf95.lastRssi();
          average_rssi = rf95.lastRssi();

        } else {
          min_rssi = min(rf95.lastRssi(), min_rssi);
          max_rssi = max(rf95.lastRssi(), max_rssi);
          average_rssi = (average_rssi * packet_req / (packet_req + 1)) + (float) rf95.lastRssi() *   1 / (packet_req + 1);

        }

        packet_req++;
      }
      else
      {
        retries++;
      }
    }
    else
    {
      retries++;
    }

  }

  if (retries >= 10) {

    Serial.println("Transmission errors.");

  } else {
    //new report
    Serial.println(" ");
    Serial.println("Pump Report:");

    uint32_t results[20];

    print_mode(result_mode[0],true);
    Serial.print(" NOW: ");
    print_time(result_time[0]);
    Serial.println("");
    Serial.println(" ");
    for (int i = 1; i < 19; i++) {
      print_mode(result_mode[i],true);
      Serial.print(" ");
      results[i] = result_time[i] - result_time[i - 1];
      print_time(results[i]);
      if ((i + 1) % 2) {

        Serial.print("  CYCLE: ");
        print_time(results[i] + results[i - 1] );
        Serial.print("     ");
      }
      else {
        Serial.print("                 ");
      }

      if (result_mode[i] == 1) Serial.print("                    ");


      print_mode(result_mode[i],false);
      print_time(results[i]);


      Serial.println("");
    }

  }
  Serial.println(" ");

  Serial.print("RSSI Min/Avg/Max ");
  Serial.print(max_rssi, 1);
  Serial.print("/");
  Serial.print(average_rssi , 1);
  Serial.print("/");
  Serial.print(min_rssi , 1);
  Serial.println(" ");
  Serial.print("Retries: ");
  Serial.println(retries, DEC);
  Serial.println(" ");
  Serial.print("Again?");
  while (!Serial.available()) {
    delay(1);
  }
  while (Serial.available()) {
    Serial.read();
  }
  Serial.println(" ");

}

void print_mode(byte status_input,bool title) {
 if(title) Serial.print("Status: ");
  if (status_input == 0) {
    Serial.print("ON  ");
  } else if (status_input == 1) {
    Serial.print("OFF ");
  } else if (status_input == 3) {
    Serial.print("PWR LOSS ");
  } else {
    Serial.print(" ? ");
  }
}

void print_time(uint32_t currentmillis)
{
  uint32_t days = 0;
  uint32_t hours = 0;
  uint32_t mins = 0;
  uint32_t secs = 0;
  secs = currentmillis / 1000; //convect milliseconds to seconds
  mins = secs / 60; //convert seconds to minutes
  hours = mins / 60; //convert minutes to hours
  days = hours / 24; //convert hours to days
  secs = secs - (mins * 60); //subtract the coverted seconds to minutes in order to display 59 secs max
  mins = mins - (hours * 60); //subtract the coverted minutes to hours in order to display 59 minutes max
  hours = hours - (days * 24); //subtract the coverted hours to days in order to display 23 hours max
  //Display results

  if (days > 0) // days will displayed only if value is greater than zero
  {
    Serial.print(days);
    Serial.print(" Days & ");
  }
  Serial.print(hours);
  Serial.print(":");
  if (mins < 10) Serial.print("0");
  Serial.print(mins);
  Serial.print(":");
  if (secs < 10) Serial.print("0");
  Serial.print(secs);

}
