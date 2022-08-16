/*
 The MIT License (MIT)
  Copyright (c) 2019 Kris Kasrpzak
  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to
  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
  the Software, and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  On a personal note, if you develop an application or product using this library
  and make millions of dollars, I'm happy for you!
*/

/*
  Code by Robert E Bridges based on Code by Kris Kasprzak kris.kasprzak@yahoo.com				https://github.com/KrisKasprzak/EBYTE
  This library is intended to be used with EBYTE transcievers, small wireless units for MCU's such as
  Teensy and Arduino. This library let's users program the operating parameters and both send and recieve data.
  This company makes several modules with different capabilities, but most #defines here should be compatible with them
  All constants were extracted from several data sheets and listed in binary as that's how the data sheet represented each setting
  Hopefully, any changes or additions to constants can be a matter of copying the data sheet constants directly into these #defines
  Usage of this library consumes around 970 bytes
  Revision		Data		Author			 Description
  1.0			3/6/2019	Kasprzak		 Initial creation
  2.0			3/2/2020	Kasprzak		 Added all functions to build the options bit (FEC, Pullup, and TransmissionMode
  3.0			3/27/2020	Kasprzak		 Added more Get functions
  4.0			6/23/2020	Kasprzak		 Added private method to clear the buffer to ensure read methods would not be filled with buffered data
  5.0			12/4/2020	Kasprzak		 moved Reset to public, added Clear to SetMode to avoid buffer corruption during programming
  5.0a			11/14/2021  Bridges			 all digitalWrites set to DigiatWriteFast, pinMode Changed from INPUT to UNPUT_PULLUP,
											 all digitalReads changed to digitalReadFast.
  1.0			12/11/2021  Bridges/Kasprzak New release for E220 module. Modified original code from Kris Kasprzak
*/

#include <EBYTE_E220.h>
#include <Stream.h>

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

uint32_t baudRates[]{ 1200, 2400, 4800, 9600, 19200, 34800, 57600, 115200 };

/*
create the transciever object
*/
EBYTE::EBYTE(Stream *s, uint8_t PIN_M0, uint8_t PIN_M1, uint8_t PIN_AUX)
{
	_s = s;
	_M0 = PIN_M0;
	_M1 = PIN_M1;
	_AUX = PIN_AUX;
}

EBYTE::callbackFunc setBaud;
uint8_t currentBaudRate = 0;
bool	autoBaud		= false;

/*
Initialize the unit--basically this reads the modules parameters and stores the parameters
for potential future module programming
*/
bool EBYTE::init(callbackFunc func = nullptr) {

	bool ok = true;
	
	autoBaud = false;

	pinMode(_AUX, INPUT_PULLUP);		//(**) pinMode Changed from INPUT to UNPUT_PULLUP
	pinMode(_M0, OUTPUT);
	pinMode(_M1, OUTPUT);

	if (func) {
		_UARTDataRate	= UDR_9600;
		currentBaudRate = _UARTDataRate;
		autoBaud		= true;
		setBaud			= func;
		setBaud(9600);
	}

	SetMode(MODE_NORMAL);

	// first get the module data (must be called first for some odd reason
	delay(100); //(**)
	
//	ok = ReadModelData();

	if (!ok) {
		return false;
	}
	// now get parameters to put unit defaults into the class variables

	ok = ReadParameters();
	if (!ok) {
		return false;
	}
	return true;
}

/*
Method to indicate availability
*/
bool EBYTE::available() {
	return _s->available();
}

/*
Method to flush serial stream
*/
void EBYTE::flush() {
	_s->flush();
}

/*
Method to write a single byte...not sure how useful this really is. If you need to send 
more that one byte, put the data into a data structure and send it in a big chunk
*/
void EBYTE::SendByte( uint8_t TheByte) {
	_s->write(TheByte);
}

/*
Method to get a single byte...not sure how useful this really is. If you need to get 
more that one byte, put the data into a data structure and send/receive it in a big chunk
*/

uint8_t EBYTE::GetByte() {
	return _s->read();
}

/*
Method to calculate noise lever in dBm from supplied RSSI data
*/
int16_t EBYTE::CalculateChannelNoiseIn_dBm(uint8_t RSSIdta) {
	return -(256 - (int16_t) RSSIdta);
};

/*
Method to send a chunk of data provided data is in a struct--my personal favorite as you 
need not parse or worry about sprintf() inability to handle floats
TTP: put your structure definition into a .h file and include in both the sender and reciever
sketches
NOTE: of your sender and receiver MCU's are different (Teensy and Arduino) caution on the data
types each handle ints floats differently
*/
bool EBYTE::SendStruct(const void *TheStructure, uint16_t size_) {

		_buf = _s->write((uint8_t *) TheStructure, size_);
		
		CompleteTask(1000);
		
		return (_buf == size_);

}

/*
Method to get a chunk of data provided data is in a struct--my personal favorite as you 
need not parse or worry about sprintf() inability to handle floats
TTP: put your structure definition into a .h file and include in both the sender and reciever
sketches
NOTE: of your sender and receiver MCU's are different (Teensy and Arduino) caution on the data
types each handle ints floats differently
*/
bool EBYTE::GetStruct(const void *TheStructure, uint16_t size_) {
	
	_buf = _s->readBytes((uint8_t*)TheStructure, size_);

	newRSSIdataAvailable = false;		//(**) Starts here until CompleteTask(1000)

	if (_EnableRSSIByte) {

		elapsedMillis t = 0;
		while ((t <= 5) && (_s->available() == 0)) {}

		newRSSIdataAvailable = (_s->available() >= 1);

		if (newRSSIdataAvailable) {
			RSSIdata = _s->read();
		}
	}
	CompleteTask(1000);

	return (_buf == size_);
}

/*
Utility method to wait until module is done tranmitting
a timeout is provided to avoid an infinite loop
*/

void EBYTE::CompleteTask(unsigned long timeout) {

	elapsedMillis t = 0;			// (**)
	
	// if AUX pin was supplied and look for HIGH state
	// note you can omit using AUX if no pins are available, but you will have to use delay() to let module finish
	if (_AUX != -1) {
		
		while (digitalReadFast(_AUX) == LOW) {    // (**) changed from digitalRead to digitalReadFast

			if (t > timeout){
				break;
			}
		}
	}
	else {				// if you can't use aux pin, use 4K7 pullup with Arduino
		delay(1000);	// you may need to adjust this value if transmissions fail
	}
	// per data sheet control after aux goes high is 2ms so delay for at least that long)
	//SmartDelay(20);
	delay(20);
}

/*
method to set the mode (program, normal, etc.)
*/
void EBYTE::SetMode(MODE_TYPE mode) {
	
	// data sheet claims module needs some extra time after mode setting (2ms)
	// most of my projects uses 10 ms, but 40ms is safer

	delay(PIN_RECOVER);
	
	if (mode == MODE_NORMAL) {
		digitalWriteFast(_M0, LOW);   // (**) all digitalWrites set to DigiatWriteFast
		digitalWriteFast(_M1, LOW);
	}
	else if (mode == MODE_WORtransmit) {    //(**) Names Changed
		digitalWriteFast(_M0, HIGH);
		digitalWriteFast(_M1, LOW);
	}
	else if (mode == MODE_WORreceive) {    //(**) Names Changed
		digitalWriteFast(_M0, LOW);
		digitalWriteFast(_M1, HIGH);
	}
	if (mode == MODE_PROGRAM) {
		digitalWriteFast(_M0, HIGH);
		digitalWriteFast(_M1, HIGH);
		if (autoBaud && (_UARTDataRate != UDR_9600)) {
			setBaud(9600);
			currentBaudRate = UDR_9600;
		}
	}
	else {
		if (autoBaud && (currentBaudRate != _UARTDataRate)) {
			setBaud( baudRates[ _UARTDataRate ] );
			currentBaudRate = _UARTDataRate;
		}
	}

	// data sheet says 2ms later control is returned, let's give just a bit more time
	// these modules can take time to activate pins
	delay(PIN_RECOVER);

	// clear out any junk
	// added rev 5
	// i've had some issues where after programming, the returned model is 0, and all settings appear to be corrupt
	// i imagine the issue is due to the internal buffer full of junk, hence clearing
	// Reset() *MAY* work but this seems better.
	ClearBuffer();


	// wait until aux pin goes back low
	CompleteTask(4000);
	lastModeSet = mode;
}

//uint8_t		_lastBaudRate		= 0;

/*
method to Set/Get the high bit of the address
*/
void EBYTE::SetAddressH(uint8_t val) {
	_AddressHigh = val;
}

uint8_t EBYTE::GetAddressH() {
	return _AddressHigh;
}

/*
method to Set/Get the lo bit of the address
*/
void EBYTE::SetAddressL(uint8_t val) {
	_AddressLow = val;
}

uint8_t EBYTE::GetAddressL() {
	return _AddressLow;
}

/*
method to Set/Get the channel
*/
void EBYTE::SetChannel(uint8_t val) {
	_Channel = val;
}
uint8_t EBYTE::GetChannel() {
	return _Channel;
}

/*
method to Set/Get the air data rate
*/
void EBYTE::SetAirDataRate(uint8_t val) {
	_AirDataRate = val;
	BuildREG0byte();
}

uint8_t EBYTE::GetAirDataRate() {
	return _AirDataRate;
}

// (**) The following functions are new since E32
/*
method to Set/Get the sub packet size
*/
void EBYTE::SetSubPacketSize(uint8_t val) {
	_SubPacketSize = val;
	BuildREG1byte();
};

uint8_t EBYTE::GetSubPacketSize() {
	return _SubPacketSize;
};

/*
method to Set/Get the RSSI Ambient Noise Enable
*/
void EBYTE::SetRSSIAmbientNoiseEnable(bool val) {
	_RSSIAmbNoiseEnable = val;
	BuildREG1byte();
};

bool EBYTE::GetRSSIAmbientNoiseEnable() {
	return _RSSIAmbNoiseEnable;
};

/*
method to Set/Get the Enable RSSI byte
*/
void EBYTE::SetEnableRSSIByte(bool val) {
	_EnableRSSIByte = val;
	BuildREG3byte();
};
bool EBYTE::GetEnableRSSIByte() {
	return _EnableRSSIByte;
};

/*
method to Set/Get the Enable LBT
*/
void EBYTE::SetEnableLBT(bool val) {
	_EnableLBT = val;
	BuildREG3byte();
};
bool EBYTE::GetEnableLBT() {
	return _EnableLBT;
};

/*
method to Set/Get the parity bit
*/
void EBYTE::SetParityBit(uint8_t val) {
	_ParityBit = val;
	BuildREG0byte();
}

uint8_t EBYTE::GetParityBit( ) {
	return _ParityBit;
}

/*
method to Set/Get Transmission Mode
*/
void EBYTE::SetTransmissionMode(uint8_t val) {
	_TransmitMode = val;
	BuildREG3byte();
}
uint8_t EBYTE::GetTransmissionMode( ) {
	return _TransmitMode;
}

/*
method to Set/Get WOR Timing
*/
void EBYTE::SetWORTIming(uint8_t val) {
	_WORTiming = val;
	BuildREG3byte();
}
uint8_t EBYTE::GetWORTIming() {
	return _WORTiming;
}

/*
method to Set/Get Transmit Power
*/
void EBYTE::SetTransmitPower(uint8_t val) {
	_TransmitPower = val;
	BuildREG1byte();
}

uint8_t EBYTE::GetTransmitPower() {
	return _TransmitPower;
}

/*
method to compute the address based on high and low bits
*/
void EBYTE::SetAddress(uint16_t Val) {
	_AddressHigh = ((Val & 0xFFFF) >> 8);
	_AddressLow = (Val & 0xFF);
}

/*
method to get the address which is a collection of hi and lo bytes
*/
uint16_t EBYTE::GetAddress() {
	return (_AddressHigh << 8) | (_AddressLow);
}

/*
Set/Get the UART baud rate
*/
void EBYTE::SetUARTBaudRate(uint8_t val) {
	_UARTDataRate = val;
	BuildREG0byte();
}

uint8_t EBYTE::GetUARTBaudRate() {
	return _UARTDataRate;
}

// (**) The following functions are new since E32
bool EBYTE::GetRSSIValues() {               // (**)

	uint8_t transaction[6]	= { 0xC0, 0xC1, 0xC2, 0xC3, 0x00, 0x02 };
	bool	ok				= false;

	if (_RSSIAmbNoiseEnable && ((lastModeSet = MODE_NORMAL) || (lastModeSet = MODE_WORtransmit))) {

		if (SendStruct(&transaction, sizeof(transaction))) {

			delay(50);
			if (_s->readBytes((uint8_t*)&transaction, 5) == 5) {
				RSSIdata		= transaction[3];
				RSSIlastReceive = transaction[4];
				ok				= true;
			};
		};

		CompleteTask(4000);
	}
	return ok;
};

/*
method to build the byte for programming (notice it's a collection of a few variables)
*/
void EBYTE::BuildREG0byte() {
	_REG0 = 0;
	_REG0 = (((_UARTDataRate & 0b111) << 5) | ((_ParityBit & 0b11) << 3) | (_AirDataRate & 0b111));
}

void EBYTE::BuildREG1byte() {
	_REG1 = 0;
	_REG1 = (((_SubPacketSize & 0b11) << 6) | ((_RSSIAmbNoiseEnable & 0b1) << 5) | (_TransmitPower & 0b11));
}

void EBYTE::BuildREG3byte() {
	_REG3 = 0;
	_REG3 = (((_EnableRSSIByte & 0b1) << 7) | ((_TransmitMode & 0b1) << 6) | ((_EnableLBT & 0b1) << 4) | (_WORTiming & 0b111));
}

bool EBYTE::GetAux() {
	return digitalReadFast(_AUX);    // (**) changed from digitalRead to digitalReadFast
}

/*
method to save parameters to the module
*/
void EBYTE::SaveParameters(PROGRAM_COMMAND_Type val) {

	config.COMMAND			= val;
	config.STARTING_ADDRESS = 0;
	config.LENGTH			= 6;
	config.ADDH				= _AddressHigh;
	config.ADDL				= _AddressLow;
	config.Reg0				= _REG0;
	config.Reg1				= _REG1;
	config.CHAN				= _Channel;
	config.Reg3				= _REG3;

	SetMode(MODE_PROGRAM);

	// here you can save permanenly or temp
	delay(5);

	if (!SendStruct(&config, sizeof(config))) {
		Serial.println(F("Unable to send Config to Tranceiver"));
	};
	elapsedMillis t = 0;                //(**)
	while ( _s->available() == 0 && t < 5000) {};

//	delay(50);   //this is a guess

//	if ( _s->readBytes((uint8_t*)&config, sizeof(config)) != sizeof(config)){
	if (!GetStruct(&config, sizeof(config))) {
		Serial.println(F("SaveParameters:Unable to Get Config from Tranceiver"));
	};

	CompleteTask(4000);
	
	SetMode(MODE_NORMAL);
}

// (**) The following function is new since E32
/*
method to save Crypt to the module
*/
void EBYTE::SetCrypt(uint16_t val) {

	uint8_t reply[6];

	_CryptHi = (uint8_t)((val & 0xFFFF) >> 8);
	_CryptLo = (uint8_t)(val & 0xFF);

	SetMode(MODE_PROGRAM);

	delay(5);

	_s->write(WRITE_CFG_PWR_DWN_SAVE);
	_s->write(0x6);				//Starting address
	_s->write(0x2);				//Length of data (number of bytes)
								//I think it's better to set Crypt seperately
	_s->write(_CryptHi);
	_s->write(_CryptLo);

	delay(50);
	if (_s->readBytes((uint8_t*)&reply, 5) != 5) {
		//	if (!getStruct(&config, sizeof(config))) {
		Serial.println(F("Unable to Set Crypt in Tranceiver"));
	};

	CompleteTask(4000);

	SetMode(MODE_NORMAL);
}

/*
method to print parameters, this can be called anytime after init(), because init gets parameters
and any method updates the variables
*/

void EBYTE::PrintParameters() {

	_UARTDataRate			= (_REG0 & 0b11100000) >> 5;
	_ParityBit				= (_REG0 & 0b00011000) >> 3;
	_AirDataRate			= (_REG0 & 0b00000111);
	_SubPacketSize			= (_REG1 & 0b11000000) >> 6;
	_RSSIAmbNoiseEnable		= (_REG1 & 0b00100000) >> 5;
	_TransmitPower			= (_REG1 & 0b00000011);

	_EnableRSSIByte			= (_REG3 & 0b10000000) >> 7;
	_TransmitMode			= (_REG3 & 0b01000000) >> 6;
	_EnableLBT				= (_REG3 & 0b00010000) >> 4;
	_WORTiming				= (_REG3 & 0b00000111);

	SerialUSB.println("----------------------------------------");
	SerialUSB.print(F("Mode (HEX/DEC/BIN): "));  SerialUSB.print(_Save, HEX); SerialUSB.print(F("/"));  SerialUSB.print(_Save, DEC); SerialUSB.print(F("/"));  SerialUSB.println(_Save, BIN);
	SerialUSB.print(F("AddH (HEX/DEC/BIN): "));  SerialUSB.print(_AddressHigh, HEX); SerialUSB.print(F("/")); SerialUSB.print(_AddressHigh, DEC); SerialUSB.print(F("/"));  SerialUSB.println(_AddressHigh, BIN);
	SerialUSB.print(F("AddL (HEX/DEC/BIN): "));  SerialUSB.print(_AddressLow, HEX); SerialUSB.print(F("/")); SerialUSB.print(_AddressLow, DEC); SerialUSB.print(F("/"));  SerialUSB.println(_AddressLow, BIN);
	SerialUSB.print(F("REG0 (HEX/DEC/BIN): "));  SerialUSB.print(_REG0, HEX); SerialUSB.print(F("/")); SerialUSB.print(_REG0, DEC); SerialUSB.print(F("/"));  SerialUSB.println(_REG0, BIN);
	SerialUSB.print(F("REG1 (HEX/DEC/BIN): "));  SerialUSB.print(_REG1, HEX); SerialUSB.print(F("/")); SerialUSB.print(_REG1, DEC); SerialUSB.print(F("/"));  SerialUSB.println(_REG1, BIN);
	SerialUSB.print(F("Chan (HEX/DEC/BIN): "));  SerialUSB.print(_Channel, HEX); SerialUSB.print(F("/")); SerialUSB.print(_Channel, DEC); SerialUSB.print(F("/"));  SerialUSB.println(_Channel, BIN);
	SerialUSB.print(F("REG3 (HEX/DEC/BIN): "));  SerialUSB.print(_REG3, HEX); SerialUSB.print(F("/")); SerialUSB.print(_REG3, DEC); SerialUSB.print(F("/"));  SerialUSB.println(_REG3, BIN);
	SerialUSB.print(F("Addr (HEX/DEC/BIN): "));  SerialUSB.print(GetAddress(), HEX); SerialUSB.print(F("/")); SerialUSB.print(GetAddress(), DEC); SerialUSB.print(F("/"));  SerialUSB.println(GetAddress(), BIN);
	SerialUSB.println(F(" "));

	SerialUSB.print(F("UARTDataRate (HEX/DEC/BIN)               : "));  SerialUSB.print(_UARTDataRate, HEX); SerialUSB.print(F("/"));  SerialUSB.print(_UARTDataRate, DEC); SerialUSB.print(F("/"));  SerialUSB.println(_UARTDataRate, BIN);
	SerialUSB.print(F("ParityBit (HEX/DEC/BIN)	                 : "));  SerialUSB.print(_ParityBit, HEX); SerialUSB.print(F("/"));  SerialUSB.print(_ParityBit, DEC); SerialUSB.print(F("/"));  SerialUSB.println(_ParityBit, BIN);
	SerialUSB.print(F("AirDataRate (HEX/DEC/BIN)                : "));  SerialUSB.print(_AirDataRate, HEX); SerialUSB.print(F("/"));  SerialUSB.print(_AirDataRate, DEC); SerialUSB.print(F("/"));  SerialUSB.println(_AirDataRate, BIN);

	SerialUSB.print(F("Packet Size (HEX/DEC/BIN)                : "));  SerialUSB.print(_SubPacketSize, HEX); SerialUSB.print(F("/"));  SerialUSB.print(_SubPacketSize, DEC); SerialUSB.print(F("/"));  SerialUSB.println(_SubPacketSize, BIN);
	SerialUSB.print(F("Enable RSSI Ambient Noise (HEX/DEC/BIN)  : "));  SerialUSB.print(_RSSIAmbNoiseEnable, HEX); SerialUSB.print(F("/"));  SerialUSB.print(_RSSIAmbNoiseEnable, DEC); SerialUSB.print(F("/"));  SerialUSB.println(_RSSIAmbNoiseEnable, BIN);
	SerialUSB.print(F("Transmit Power (HEX/DEC/BIN)             : "));  SerialUSB.print(_TransmitPower, HEX); SerialUSB.print(F("/"));  SerialUSB.print(_TransmitPower, DEC); SerialUSB.print(F("/"));  SerialUSB.println(_TransmitPower, BIN);

	SerialUSB.print(F("Enable RSSI byte (HEX/DEC/BIN)           : "));  SerialUSB.print(_EnableRSSIByte, HEX); SerialUSB.print(F("/"));  SerialUSB.print(_EnableRSSIByte, DEC); SerialUSB.print(F("/"));  SerialUSB.println(_EnableRSSIByte, BIN);
	SerialUSB.print(F("TransMode (HEX/DEC/BIN)                  : "));  SerialUSB.print(_TransmitMode, HEX); SerialUSB.print(F("/"));  SerialUSB.print(_TransmitMode, DEC); SerialUSB.print(F("/"));  SerialUSB.println(_TransmitMode, BIN);
	SerialUSB.print(F("Enable LBT (HEX/DEC/BIN)                 : "));  SerialUSB.print(_EnableLBT, HEX); SerialUSB.print(F("/"));  SerialUSB.print(_EnableLBT, DEC); SerialUSB.print(F("/"));  SerialUSB.println(_EnableLBT, BIN);
	SerialUSB.print(F("WOR Timing (HEX/DEC/BIN)                 : "));  SerialUSB.print(_WORTiming, HEX); SerialUSB.print(F("/"));  SerialUSB.print(_WORTiming, DEC); SerialUSB.print(F("/"));  SerialUSB.println(_WORTiming, BIN);

	SerialUSB.println("----------------------------------------");

}

/*
method to read parameters, 
*/
bool EBYTE::ReadParameters() {

	config.COMMAND			= READ_CONFIGURATION;
	config.STARTING_ADDRESS = 0;
	config.LENGTH			= 6;
	config.ADDH				= _AddressHigh;
	config.ADDL				= _AddressLow;
	config.Reg0				= _REG0;
	config.Reg1				= _REG1;
	config.CHAN				= _Channel;
	config.Reg3				= _REG3;

	SetMode(MODE_PROGRAM);

	if (!SendStruct(&config, 3)) {
		Serial.println(F("Unable to send Config to Tranceiver"));
	};

	delay(50);   //this is a guess

	if (_s->readBytes((uint8_t*)&config, sizeof(config)) != sizeof(config)) {
		Serial.println(F("ReadParameteres: Unable to Get Config from Tranceiver"));
	};

	_Save					= config.COMMAND;
	_AddressHigh			= config.ADDH;
	_AddressLow				= config.ADDL;
	_REG0					= config.Reg0;
	_REG1					= config.Reg1;
	_Channel				= config.CHAN;
	_REG3					= config.Reg3;

	_Address				=  (_AddressHigh << 8) | (_AddressLow);
	_UARTDataRate			= (_REG0 & 0b11100000) >> 5;
	_ParityBit				= (_REG0 & 0b00011000) >> 3;
	_AirDataRate			= (_REG0 & 0b00000111);

	_SubPacketSize			= (_REG1 & 0b11000000) >> 6;        //(**)
	_RSSIAmbNoiseEnable		= (_REG1 & 0b00100000) >> 4;
	_TransmitPower			= (_REG1 & 0b00000011);

	_EnableRSSIByte			= (_REG3 & 0b10000000) >> 7;
	_TransmitMode			= (_REG3 & 0b01000000) >> 6;
	_EnableLBT				= (_REG3 & 0b00010000) >> 4;
	_WORTiming				= (_REG3 & 0b00000111);
	
	SetMode(MODE_NORMAL);

	if (_Save != RETURNED_COMMAND){
		return false;
	}
	return true;	
}

/*
method to clear the serial buffer

without clearing the buffer, i find getting the parameters very unreliable after programming.
i suspect stuff in the buffer affects programming 
hence, let's clean it out
this is called as part of the setmode

*/
void EBYTE::ClearBuffer(){

	unsigned long amt = millis();

	while(_s->available()) {
		_s->read();
		if ((millis() - amt) > 5000) {
          Serial.println(F("runaway"));
          break;
        }
	}

}
