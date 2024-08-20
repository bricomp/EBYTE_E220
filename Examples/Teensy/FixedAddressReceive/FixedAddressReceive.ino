/*

  This example shows how to connect to an EBYTE transceiver
  using a Teensy 3.2

  This code for for the receiver


  connections
  Module      Teensy
  M0          2
  M1          3
  Rx          1 (MCU Tx line)
  Tx          0 (MCU Rx line)
  Aux         4
  Vcc         3V3
  Gnd         Gnd

*/

#include "EBYTE_E220.h"

// connect to any of the Teensy Serial ports
#define ESerial Serial1

#define PIN_M0 2
#define PIN_M1 3
#define PIN_AX 4

#define myAddrL 2
#define myAddrH 0
#define myChan  15
// i recommend putting this code in a .h file and including it
// from both the receiver and sender modules

// these are just dummy variables, replace with your own
struct DATA {
  unsigned long Count;
  int Bits;
  float Volts;
  float Amps;

};

int Chan;
DATA MyData;
unsigned long Last;

// create the transceiver object, passing in the serial and pins
EBYTE_E220 Transceiver(&ESerial, PIN_M0, PIN_M1, PIN_AX);

void setup() {

  Serial.begin(9600);

  // wait for the serial to connect
  while (!Serial) {}

  // start the transceiver serial port--i have yet to get a different
  // baud rate to work--data sheet says to keep on 9600

  ESerial.begin(9600);

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

  //  Transceiver.SetAddressH(4);
  //  Transceiver.SetAddressL(0);
  //  Transceiver.SetChannel(Chan);
  //  save the parameters to the unit,
  //   Transceiver.SaveParameters(PERMANENT);

  // you can print all parameters and is good for debugging
  // if your units will not communicate, print the parameters
  // for both sender and receiver and make sure air rates, channel
  // and address is the same
  //  Transceiver.PrintParameters();

  //   Transceiver.GetRSSIValues();
  //   Serial.print("RSSI                  : "); Serial.println(Transceiver.RSSIdata);
  //   Serial.print("RSSI on Last Receive  : "); Serial.println(Transceiver.RSSIlastReceive);
   Transceiver.SetAddressH( myAddrH );
   Transceiver.SetAddressL( myAddrL );
   Transceiver.SetChannel(  myChan  );
   Transceiver.SetTransmissionMode(FixedModeENABLE);
   Transceiver.PrintParameters();
   Transceiver.SaveParameters(PERMANENT);
//   Transceiver.GetRSSIValues();
//   Serial.print("RSSI                  : "); Serial.println(Transceiver.RSSIdata);
//   Serial.print("RSSI on Last Receive  : "); Serial.println(Transceiver.RSSIlastReceive);
}

void loop() {

  // if the transceiver serial is available, proces incoming data
  // you can also use ESerial.available()
  if (Transceiver.available()) {

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
    if ((millis() - Last) > 10000) {
      Serial.println("Searching: ");
      Last = millis();
    }

  }

}
