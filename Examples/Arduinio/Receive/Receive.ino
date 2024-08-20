/*

  This example shows how to connect to an EBYTE transceiver
  using an Arduino Nano

  This code for for the sender


  connections
  Module      Nano
  M0          4
  M1          5
  Rx          2 (MCU Tx line)
  Tx          3 (MCU Rx line)
  Aux         6
  Vcc         3V3
  Gnd         Gnd

*/

#include <SoftwareSerial.h>
#include "EBYTE_E220.h"

#define PIN_RX 2
#define PIN_TX 3
#define PIN_M0 4
#define PIN_M1 5
#define PIN_AX 6


// i recommend putting this code in a .h file and including it
// from both the receiver and sender modules
struct DATA {
  unsigned long Count;
  int Bits;
  float Volts;
  float Amps;

};

// these are just dummy variables, replace with your own
int Chan;
DATA MyData;
unsigned long Last;

// connect to any digital pin to connect to the serial port
// don't use pin 01 and 1 as they are reserved for USB communications
SoftwareSerial ESerial(PIN_RX, PIN_TX);

// create the transceiver object, passing in the serial and pins
EBYTE_E220 Transceiver(&ESerial, PIN_M0, PIN_M1, PIN_AX);

void setup() {


  Serial.begin(9600);

  ESerial.begin(9600);
  Serial.println("Starting Reader");

  // this init will set the pinModes for you
  Transceiver.init();
  // The following call is optional but can be very useful when two Modules will not talk to themselves.
  // It is quite possible that one Module has had a parameter set which is different from the other.
  // Under these circumstances the Modules will NOT talk to each other and it is not at first sight obvious why.
  // Using the Function below has saved me many an hour chasing a program error which was not there. I talk from Experience.

  Transceiver.SetDefaultParameters();

  // all these calls are optional but shown to give examples of what you can do
//   Transceiver.SetMode();
//   void	SetAddress(uint16_t val = 0);
//   Transceiver.SetAddressH(0);
//   Transceiver.SetAddressL(0);
   //REG0
//   Transceiver.SetUARTBaudRate(UDR_9600);
//   Transceiver.SetParityBit(PB_8N1);
//   Transceiver.SetAirDataRate(ADR_2400);
   //REG1
//   Transceiver.SetSubPacketSize(PKT_200bytes);
//   Transceiver.SetRSSIAmbientNoiseEnable(RSSI_Disable);
//   Transceiver.SetTransmitPower(PWR_TP22);
   //RETransceiver.G2
//   Transceiver.SetChannel(15);
   //REG3
//   Transceiver.SetEnableRSSIByte(RSSIDisable);
//   Transceiver.SetTransmissionMode(FixedModeDISABLE);
//   Transceiver.SetEnableLBT(LBTDisable);
//   Transceiver.SetWORTIming(OPT_WAKEUP500);

//   Transceiver.SetCrypt(0);

  Transceiver.SetAddressH(4);
  Transceiver.SetAddressL(0);
  Chan = 15;
  Transceiver.SetChannel(Chan);
  //  save the parameters to the unit,
 //   Transceiver.SaveParameters(PERMANENT);

   // you can print all parameters and is good for debugging
   // if your units will not communicate, print the parameters
   // for both sender and receiver and make sure air rates, channel
   // and address is the same
  Transceiver.PrintParameters();

  //   Transceiver.GetRSSIValues();
  //   Serial.print("RSSI                  : "); Serial.println(Transceiver.RSSIdata);
  //   Serial.print("RSSI on Last Receive  : "); Serial.println(Transceiver.RSSIlastReceive);
}

void loop() {

  // if the transceiver serial is available, proces incoming data
  // you can also use Transceiver.available()


  if (ESerial.available()) {

    // i highly suggest you send data using structures and not
    // a parsed data--i've always had a hard time getting reliable data using
    // a parsing method

    Transceiver.GetStruct(&MyData, sizeof(MyData));

    // dump out what was just received
    Serial.print("Count: "); Serial.println(MyData.Count);
    Serial.print("Bits: "); Serial.println(MyData.Bits);
    Serial.print("Volts: "); Serial.println(MyData.Volts);
    // if you got data, update the checker
    Last = millis();

  }
  else {
    // if the time checker is over some prescribed amount
    // let the user know there is no incoming data
    if ((millis() - Last) > 1000) {
      Serial.println("Searching: ");
      Last = millis();
    }

  }
}
