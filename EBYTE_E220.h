#pragma once
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
  1.0			12/07/2021  Bridges/Kasprzak New release for E220 module. Modified original code from Kris Kasprzak
  1.0a			12/04/2023  Bridges			 Small update to stop compiler warnings. Has no effect on performance. Affects .cpp file.

  *******************************************************************
  **                                                               **
  ** NOTE CHANGES FROM E32 ARE GENERALY IDENTIFIED BY (**) COMMENT **
  **    IN BOTH .H AND .CPP FILES. NOT ALL I GOT A BIT BORED!!     **
  **                                                               **
  *******************************************************************
   
  Module connection
  Module	MCU						Description
  MO		Any digital pin*		pin to control working/program modes
  M1		Any digital pin*		pin to control working/program modes
  Rx		Any digital pin			pin to MCU TX pin (module transmits to MCU, hence MCU must recieve data from module
  Tx		Any digital pin			pin to MCU RX pin (module transmits to MCU, hence MCU must recieve data from module
  AUX		Any digital pin			pin to indicate when an operation is complete (low is busy, high is done)
  Vcc		+3v3 or 5V0				
  Vcc		Ground					Ground must be common to module and MCU		
  notes:
  * caution in connecting to Arduino pin 0 and 1 as those pins are for USB connection to PC
  you may need a 4K7 pullup to Rx and AUX pins (possibly Tx) if using and Arduino
  Module source
  http://www.ebyte.com/en/
  example module this library is intended to be used with
  http://www.ebyte.com/en/product-view-news.aspx?id=174
  Code usage
  1. Create a serial object
  2. Create EBYTE object that uses the serail object
  3. begin the serial object
  4. init the EBYTE object
  5. set parameters (optional but required if sender and reciever are different)
  6. send or listen to sent data
  
*/

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

// if you seem to get "corrupt settings add this line to your .ino
// #include <avr/io.h>

/* 
if modules don't seem to save, you will have to adjust this value
when settin M0 an M1 there is gererally a short time for the transceiver modules
to react, some say only 10 ms, but I've found it can be much lonnger, I'm using
100 ms below and maybe too long, but it seemed to work in my cases
*/
#define PIN_RECOVER 15 

// modes NORMAL send and recieve for example	Changed to ENUM (**)
enum MODE_TYPE {
	MODE_NORMAL		 = 0,		// can send and recieve
	MODE_WORtransmit = 1,		// sends a preamble to waken receiver
	MODE_WORreceive  = 2,		// can't transmit but receive works only in wake up mode
	MODE_PROGRAM	 = 3,		// for programming
	MODE_DEEPSLEEP	 = 3,		//(**) Same as programming but better for program readability
	MODE_NOT_SET	 = 0xFF		//(**)
};

// (**)
enum PROGRAM_COMMAND_Type {
	WRITE_CFG_PWR_DWN_SAVE	= 0xC0,
	READ_CONFIGURATION		= 0xC1,
	WRITE_CFG_PWR_DWN_LOSE	= 0xC2,
	WRONG_FORMAT			= 0xFF,
	RETURNED_COMMAND		= 0xC1,

//	SPECIAL_WIFI_CONF_COMMAND = 0xCF    //not E220 but E22
};
// options to save change permanently or temp (power down and restart will restore settings to last saved options

#define PERMANENT WRITE_CFG_PWR_DWN_SAVE
#define TEMPORARY WRITE_CFG_PWR_DWN_LOSE
/***************
**    REG0    **
****************/

//UART data rates
// (can be different for transmitter and reveiver)
//REG0			     xxx_ ____

#define UDR_1200   0b000		// 1200 baud
#define UDR_2400   0b001		// 2400 baud
#define UDR_4800   0b010		// 4800 baud
#define UDR_9600   0b011		// 9600 baud default
#define UDR_19200  0b100		// 19200 baud
#define UDR_38400  0b101		// 34800 baud
#define UDR_57600  0b110		// 57600 baud
#define UDR_115200 0b111		// 115200 baud

// parity bit options (can be different for transmitter and reveiver)
//REG0       ___x x___
#define PB_8N1 0b00			// default
#define PB_8O1 0b01
#define PB_8E1 0b10				//(**) change/correct

// air data rates (certian types of modules)
// (must be the same for transmitter and reveiver)
//REG0        ____ _xxx
#define ADR_2400a 0b000		// 2400 baud     //(**)
#define ADR_2400b 0b001		// 2400 baud	 //(**)
#define ADR_2400  0b010		// 2400 baud
#define ADR_4800  0b011		// 4800 baud
#define ADR_9600  0b100		// 9600 baud
#define ADR_19200 0b101		// 19200 baud
#define ADR_34800 0b110		// 34800 baud	//(**)
#define ADR_62500 0b111		// 62500 baud	//(**)

/*
// air data rates (other types of modules)
#define ADR_1K 0b000		// 1k baud
#define ADR_2K 0b001		// 2K baud
#define ADR_5K 0b010		// 4K baud
#define ADR_8K 0b011		// 8K baud
#define ADR_10K 0b100		// 10K baud
#define ADR_15K 0b101		// 15K baud
#define ADR_20K 0b110		// 20K baud
#define ADR_25K 0b111		// 25K baud
*/
/***************
**    REG1    **
****************/

//   Sub packet size
//REG1                 xx__ ____
#define PKT_200bytes 0b00		//default
#define PKT_128bytes 0b01
#define PKT_64bytes  0b10
#define PKT_32bytes	 0b11

//  RSSI Ambient noise enable
 
//REG1             __x_ ____
#define RSSI_Disable 0b0			//default
#define RSSI_Enable  0b1

//REG1            ___x xx__   Reserved
// 
// transmitter output power--check government regulations on legal transmit power
// refer to the data sheet as not all modules support these power levels

// (can be different for transmitter and reveiver)
// Values for 22dB unit
//REG1      ____ __xx
#define PWR_TP22 0b00		// 22 db  default
#define PWR_TP17 0b01		// 17 db
#define PWR_TP13 0b10		// 13 db
#define PWR_TP10 0b11		// 1o db

// Values for 30dB unit
//REG1      ____ __xx
#define PWR_TP30 0b00		// 30 db  default
#define PWR_TP27 0b01		// 27 db
#define PWR_TP24 0b10		// 24 db
#define PWR_TP21 0b11		// 21 db

/***************
**    REG2    **
****************/

//REG2 is Channel - set elsewhere

/***************
**    REG3    **
****************/

// Enable RSSI Byte
//REG3              x___ ____
#define RSSIEnable  0b0
#define RSSIDisable 0b1

// (can be different for transmitter and reveiver)
//REG3                    _x__ ____
#define FixedModeDISABLE 0b0		// Names Changed by REB to make them more understandable    <<default	(**)
#define FixedModeENABLE  0b1		// Names Changed by REB to make them more understandable				(**)

//REG3                    __x_ ____    Reserved

//LBT Enable
//REG3                 ___x ____
#define LBTDisable		0b0			//default
#define LBTEnable		0b1

//REG3                ____ x___    Reserved

// (transmitter and receiver MUST be the same)  (**)
//REG3             ____ _xxx
#define OPT_WAKEUP500  0b000			// (**) Effectively all changed from E32
#define OPT_WAKEUP1000 0b001
#define OPT_WAKEUP1500 0b010
#define OPT_WAKEUP2000 0b011
#define OPT_WAKEUP2500 0b100
#define OPT_WAKEUP3000 0b101
#define OPT_WAKEUP3500 0b110
#define OPT_WAKEUP4000 0b111

class Stream;

class EBYTE {

public:

	EBYTE(Stream *s, uint8_t PIN_M0 = 4, uint8_t PIN_M1 = 5, uint8_t PIN_AUX = 6);

	// code to initialize the library
	// this method reads all parameters from the module and stores them in memory
	// library modifications could be made to only read upon a change at a savings of 30 or so bytes
	// the issue with these modules are some parameters are a collection of several options AND
	// ALL parameters must be sent even if only one option is changed--hence get all parameters initially
	// so you know what the non changed parameters are know for resending back

	typedef void (*ebyteCallbackFunc) (uint32_t);					//create function pointer type

	bool	init(ebyteCallbackFunc func = nullptr);

	// methods to set modules working parameters NOTHING WILL BE SAVED UNLESS SaveParameters() is called
	void	SetMode(MODE_TYPE mode = MODE_NORMAL);
	void	SetAddress(uint16_t val = 0);
	void	SetAddressH(uint8_t val = 0);
	void	SetAddressL(uint8_t val = 0);
//REG0
	void	SetUARTBaudRate(uint8_t val);
	void	SetParityBit(uint8_t val);
	void	SetAirDataRate(uint8_t val);
//REG1
	void	SetSubPacketSize(uint8_t val);
	void	SetRSSIAmbientNoiseEnable(bool val);
	void	SetTransmitPower(uint8_t val);
//REG2
	void	SetChannel(uint8_t val);
//REG3
	void	SetEnableRSSIByte(bool val);
	void	SetTransmissionMode(uint8_t val);
	void	SetEnableLBT(bool val);
	void	SetWORTIming(uint8_t val);

	void	SetCrypt(uint16_t val);

	bool	GetAux();

	bool	available();
	void	flush();

/*************************************************
*	 methods to get some operating parameters    *
**************************************************/

	uint16_t GetAddress();

	// methods to get module data
//	uint8_t GetModel();        (**) Not in E220

	uint8_t GetAddressH();
	uint8_t GetAddressL();
//REG0
	uint8_t GetUARTBaudRate();
	uint8_t GetParityBit();
	uint8_t GetAirDataRate();
//REG1
	uint8_t GetSubPacketSize();
	bool	GetRSSIAmbientNoiseEnable();
	uint8_t GetTransmitPower();
//REG2
	uint8_t GetChannel();
//REG3
	bool	GetEnableRSSIByte();
	uint8_t GetTransmissionMode();
	bool	GetEnableLBT();
	uint8_t GetWORTIming();

	// Method to get RSSIdata and RSSIlastReceive if _RSSIAmbNoiseEnable turned on and mode is MODE_NORMAL OR MODE_WAKEUP
	bool	GetRSSIValues();

	// methods to get data from sending unit
	uint8_t GetByte();

	// Method to get structured data. If EnableRSSIByte is true then the RSSIbyte will be read and placed in the variable RSSIdata
	bool	GetStruct(const void *TheStructure, uint16_t size_);  // Gets struct data and RSSIdata if sender _EnableRSSIByte turned on.
	
	// method to send to data to receiving unit
	void	SendByte(uint8_t TheByte);
	bool	SendStruct(const void *TheStructure, uint16_t size_);
	
	// mehod to print parameters
	void	PrintParameters();
	
	// parameters are set above but NOT saved, here's how you save parameters
	// notion here is you can set several but save once as opposed to saving on each parameter change
	// you can save permanently (retained at start up, or temp which is ideal for dynamically changing the address or frequency
	void SaveParameters(PROGRAM_COMMAND_Type val = PERMANENT);
	
	uint8_t RSSIdata		= 0;    // store for RSSIdata received when _EnableRSSIByte is true or from GetRSSIValues()
	uint8_t RSSIlastReceive = 0;	// returned from GetRSSIValues(). Value of RSSI on last receive.

	// indicates whether RSSI data has been received in RSSIdata when _EnableRSSIByte is true
	bool	newRSSIdataAvailable = false;

	// Method to calculate noise lever in dBm from supplied RSSI data
	int16_t CalculateChannelNoiseIn_dBm(uint8_t RSSIdta);

	// NOT AVAILABLE IN E220
	// MFG is not clear on what Reset does, but my testing indicates it clears buffer
	// I use this when needing to restart the EBYTE after programming while data is still streaming in
	// it does NOT return the ebyte back to factory defaults
//	void Reset();

protected:

	// function to read modules parameters
	bool ReadParameters();

	// method to let method know of module is busy doing something (timeout provided to avoid lockups)
	void CompleteTask(unsigned long timeout = 0);
	
/*
	Utility methodS to build the bytes for programming (notice it's a collection of a few variables)
*/
	void BuildREG0byte();
	void BuildREG1byte(); 
	void BuildREG3byte();

private:

//	bool ReadModelData();		//(**) Not available on E220
	void ClearBuffer();
	// variable for the serial stream
	Stream*  _s;
	Stream*  _TD;

	// pin variables
	int8_t _M0;
	int8_t _M1;
	int8_t _AUX;

	MODE_TYPE lastModeSet = MODE_NOT_SET;

#pragma pack(push,1)

	struct ConfigurationType {
		byte COMMAND = 0;
		byte STARTING_ADDRESS = 0;
		byte LENGTH = 0;

		byte ADDH = 0;
		byte ADDL = 0;
//		byte NETID = 0;		//Used in E22

		byte Reg0;
		byte Reg1;

		byte CHAN = 0;
		byte Reg3;

	};
	ConfigurationType config;

#pragma pack(pop)

	// indicidual variables for each of the 6 bytes
	// _Params could be used as the main variable storage, but since some bytes
	// are a collection of several options, let's just make storage consistent
	// also Param[1] is different data depending on the _Save variable
	uint8_t		_Save;
	uint8_t		_AddressHigh;
	uint8_t		_AddressLow;
	uint8_t		_REG0;
	uint8_t		_REG1;
	uint8_t		_REG2;
	uint8_t		_Channel;	//Same as REG2
	uint8_t		_REG3;
	uint8_t		_CryptHi;
	uint8_t		_CryptLo;

	// individual variables for all the options
	uint8_t		_UARTDataRate;					//REG0 xxx_ ____
	uint8_t		_ParityBit;						//REG0 ___x x___
	uint8_t		_AirDataRate;					//REG0 ____ _xxx
	uint8_t		_SubPacketSize;					//REG1 xx_. ..__    "." = Reserved bit
	bool		_RSSIAmbNoiseEnable = false;	//REG1 __x. ..__
	uint8_t		_TransmitPower;					//REG1 ____ __xx
	bool		_EnableRSSIByte		= true;		//REG3 x_._ .___	Set true to catch any extra bytes sent while being set-up and possibly not fully under control
	uint8_t		_TransmitMode;					//REG3 _x._ .___
	bool		_EnableLBT			= false;	//REG3 __.x .___
	uint8_t		_WORTiming;						//REG3 __._ .xxx
	uint16_t	_Address;
	uint8_t		_buf;

};

