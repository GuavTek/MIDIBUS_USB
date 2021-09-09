/*
 * MCP2517.h
 *
 * Created: 01/07/2021 17:36:38
 *  Author: GuavTek
 */ 


#ifndef MCP2517_H_
#define MCP2517_H_

#include "SPI.h"

enum class MCP2517_INSTR_E {
	Reset			= 0b0000,
	Read			= 0b0011,
	Write			= 0b0010,
	Read_CRC		= 0b1011,
	Write_CRC		= 0b1010,
	Write_Safe		= 0b1100
};

enum class MCP2517_ICODE_E {
	TXQ = 0x00,
	FIFO1 = 0x01,
	FIFO2 = 0x02,
	FIFO3 = 0x03,
	FIFO4 = 0x04,
	FIFO5 = 0x05,
	FIFO6 = 0x06,
	FIFO7 = 0x07,
	FIFO8 = 0x08,
	FIFO9 = 0x09,
	FIFO10 = 0x0a,
	FIFO11 = 0x0b,
	FIFO12 = 0x0c,
	FIFO13 = 0x0d,
	FIFO14 = 0x0e,
	FIFO15 = 0x0f,
	FIFO16 = 0x10,
	FIFO17 = 0x11,
	FIFO18 = 0x12,
	FIFO19 = 0x13,
	FIFO20 = 0x14,
	FIFO21 = 0x15,
	FIFO22 = 0x16,
	FIFO23 = 0x17,
	FIFO24 = 0x18,
	FIFO25 = 0x19,
	FIFO26 = 0x1a,
	FIFO27 = 0x1b,
	FIFO28 = 0x1c,
	FIFO29 = 0x1d,
	FIFO30 = 0x1e,
	FIFO31 = 0x1f,
	NoInt = 0x80,
	ErrorInt = 0x81,
	WakeUp = 0x82,
	RxOverflow = 0x83,
	AddressError = 0x84,
	MABFlow = 0x85,
	TBCOverflow = 0x86,
	OpModeChanged = 0x87,
	InvalidMessage = 0x88,
	TxEvent = 0x89,
	TxAttempt = 0x8a
};

// C1TREX transmit request
// C1RXIF receive interrupt pending register
// C1TREC and C1BDIAG for debugging
// C1TXCON transmit queue control
// C1TXQUA read transmit queue head

struct CAN_Rx_msg_t {
	uint32_t id;
	uint8_t filterHit;
	bool errorStatus;
	bool idExtend;
	uint8_t dataLength;
};



class MCP2517_C : SPI_C {
	public:
		void Init(uint8_t intPin, const spi_config_t config);
		inline void Set_Rx_Callback(void (*cb)(char*)){ Rx_Callback = cb; }
		uint8_t Transmit_Message(char* data, uint8_t length, bool broadcast);
		void State_Machine();
		inline void Handler();
		enum ADDR_E : uint16_t;
		using SPI_C::SPI_C;
	protected:
		void Reset();
		void Generate_CAN_ID();
		void Write_Word_Blocking(enum ADDR_E addr, uint32_t data);
		uint32_t Receive_Word_Blocking(enum ADDR_E addr);
		uint8_t Send_Buffer(enum ADDR_E addr, char* data, uint8_t length);
		uint8_t Receive_Buffer(enum ADDR_E addr, uint8_t length);
		void (*Rx_Callback)(char*);
		void Check_Rx_Int();
		void Check_Rx_RTC();
		inline uint8_t Get_DLC(uint8_t dataLength);
		inline uint8_t Get_Data_Length(uint8_t DLC);
		inline uint16_t Get_FIFOCON_Addr(uint8_t fifoNum);
		inline uint16_t Get_FIFOSTA_Addr(uint8_t fifoNum);
		inline uint16_t Get_FIFOUA_Addr(uint8_t fifoNum);
		void Send_Message_Object(uint16_t addr);
		void FIFO_Increment(uint8_t fifoNum, uint8_t txRequest);
		void FIFO_User_Address(uint8_t fifoNum);
		void FIFO_Status(uint8_t fifoNum);
		uint8_t interruptPin;
		uint16_t CANID;
		bool broadcasting;
		uint8_t payload;
		enum {Msg_Idle = 0,
			Msg_Rx_Int, Msg_Rx_Addr, Msg_Rx_Data, Msg_Rx_FIFO,
			Msg_Tx_Addr, Msg_Tx_Data, Msg_Tx_FIFO} msgState;
};

enum MCP2517_C::ADDR_E : uint16_t {
	C1CON			= 0x000,
	C1NBTCFG		= 0x004,
	C1DBTCFG		= 0x008,
	C1TDC			= 0x00c,
	C1TBC			= 0x010,
	C1TSCON			= 0x014,
	C1VEC			= 0x018,
	C1INT			= 0x01c,
	C1RXIF			= 0x020,
	C1TXIF			= 0x024,
	C1RXOVIF		= 0x028,
	C1TXATIF		= 0x02c,
	C1TXREQ			= 0x030,
	C1TREC			= 0x034,
	C1BDIAG0		= 0x038,
	C1BDIAG1		= 0x03c,
	C1TEFCON		= 0x040,
	C1TEFSTA		= 0x044,
	C1TEFUA			= 0x048,
	C1TXQCON		= 0x050,
	C1TXQSTA		= 0x054,
	C1TXQUA			= 0x058,
	C1FIFOCON1		= 0x05c,
	C1FIFOSTA1		= 0x060,
	C1FIFOUA1		= 0x064,
	C1FIFOCON2		= 0x068,
	C1FIFOSTA2		= 0x06c,
	C1FIFOUA2		= 0x070,
	C1FLTCON0		= 0x1d0,
	C1FLTCON1		= 0x1d4,
	C1FLTCON2		= 0x1d8,
	C1FLTCON3		= 0x1dc,
	C1FLTCON4		= 0x1e0,
	C1FLTCON5		= 0x1e4,
	C1FLTCON6		= 0x1e8,
	C1FLTCON7		= 0x1ec,
	C1FLTOBJ0		= 0x1f0,
	C1MASK0			= 0x1f4,
	C1FLTOBJ1		= 0x1f8,
	C1MASK1			= 0x1fc,
	RAM_START		= 0x400,
	RAM_END			= 0xbff,
	OSC				= 0xe00,
	IOCON			= 0xe04,
	CRC				= 0xe08,
	ECCCON			= 0xe0c,
	ECCSTAT			= 0xe10
};

#warning CAN is set up in loopback mode
// Initializes the MCP2517 chip
// intPin > 64 makes it use RTC to check RX
void MCP2517_C::Init(uint8_t intPin, const spi_config_t config){
	SPI_C::Init(config);
	
	Reset();
	Generate_CAN_ID();
	
	interruptPin = intPin;
	if (intPin < 64){
		struct port_config intCon = {
			.direction = PORT_PIN_DIR_INPUT,
			.input_pull = PORT_PIN_PULL_NONE,
			.powersave = false
		};
		port_pin_set_config(intPin, &intCon);
	}
	
	
	uint32_t temp;
	temp = 0;
	temp |= 1 << 27;		// Abort all pending transmissions
	temp |= 0b100 << 24;	// Request config mode
	
	// Wait for config mode
	do {
		temp = Receive_Word_Blocking(ADDR_E::C1CON);
	} while ((temp & (0b111 << 21)) != (0b100 << 21));
	
	
	
	// Configure message filter 1
	temp = 0;
	temp |= 1 << 30;		// Match only standard ID
	temp |= 0x7ff << 0;		// Mask for standard identifier
	Write_Word_Blocking(ADDR_E::C1MASK1, temp);
	
	temp = 0;
	temp |= 0 << 30;		// Standard ID only
	temp |= 1 << 10;		// Always 1 when this module is addressed
	temp |= CANID & 0x3ff;	// Address
	Write_Word_Blocking(ADDR_E::C1FLTOBJ1, temp);
	
	// Configure message filter 0
	temp = 0;
	temp |= 1 << 30;		// Match only standard ID
	temp |= 1 << 10;		// Mask for standard identifier
	Write_Word_Blocking(ADDR_E::C1MASK0, temp);
	
	temp = 0;
	temp |= 0 << 30;		// Standard ID only
	temp |= 0 << 10;		// All addresses starting with 0 are accepted
	Write_Word_Blocking(ADDR_E::C1FLTOBJ0, temp);
	
	// Enable filters
	temp = 0;
	temp |= 1 << 15;		// Enable filter 1
	temp |= 0b0001 << 8;	// Store in FIFO 1
	temp |= 1 << 7;			// Enable filter 0
	temp |= 0b0001 << 0;	// Store in FIFO 1
	Write_Word_Blocking(ADDR_E::C1FLTCON0, temp);
	
	// Configure Rx FIFO 1
	temp = 0;
	temp |= 0b010 << 29;	// Payload size = 16 bytes
	temp |= 0b11111 << 24;	// FIFO size = 32 messages
	temp |= 0 << 7;			// FIFO is receive
	temp |= 1 << 0;			// FIFO not empty interrupt enable
	Write_Word_Blocking(ADDR_E::C1FIFOCON1, temp);
	
	// Configure Tx FIFO 2
	temp = 0;
	temp |= 0b010 << 29;	// Payload size = 16 bytes
	temp |= 0b11111 << 24;	// FIFO size = 32 messages
	temp |= 1 << 7;			// FIFO is transmit
	temp |= 0b11 << 21;		// Unlimited retransmission attempts
	Write_Word_Blocking(ADDR_E::C1FIFOCON2, temp);
	
	// Configure interrupts
	temp = 0;
	//temp |= 1 << 20;		// Tx event interrupt enable, Flag is in position 4
	temp |= 1 << 17;		// Rx interrupt enable, Flag is in position 1
	Write_Word_Blocking(ADDR_E::C1INT, temp);
	
	temp = 0;
	temp |= 1 << 25;		// Enable edge filtering
	temp |= 0b11 << 16;		// Auto delay compensation
	Write_Word_Blocking(ADDR_E::C1TDC, temp);
	
	temp = 0;
	temp |= 0x01 << 24;		// Time quanta prescaler Tq = 2/20MHz = 100ns
	temp |= 0x05 << 16;		// Time segment 1 = 6 Tq
	temp |= 0x02 << 8;		// Time segment 2 = 3 Tq
	temp |= 0x00;			// Sync jump width = 1 Tq
	Write_Word_Blocking(ADDR_E::C1NBTCFG, temp);
	Write_Word_Blocking(ADDR_E::C1DBTCFG, temp);
	
	temp = 0;
	temp |= 0b0010 << 28;	// 4 cycle delay between transmissions
	//temp |= 0b000 << 24;	// Normal CAN FD mode
	temp |= 0b010 << 24;		// Internal loopback mode
	temp |= 0 << 12;		// Enable bit-rate switching
	temp |= 1 << 5;			// Use non-zero initial crc vector
	Write_Word_Blocking(ADDR_E::C1CON, temp);
	
	// Wait for chosen mode
	do {
		temp = Receive_Word_Blocking(ADDR_E::C1CON);
	} while ((temp & (0b111 << 21)) != (0b010 << 21));
	
}

inline uint8_t MCP2517_C::Get_DLC(uint8_t dataLength){
	switch(dataLength){
		case 0:
			return 0;
		case 1:
			return 1;
		case 2:
			return 2;
		case 3:
			return 3;
		case 4:
			return 4;
		case 5:
			return 5;
		case 6:
			return 6;
		case 7:
			return 7;
		case 8:
			return 8;
		case 12:
			return 9;
		case 16:
			return 10;
		case 20:
			return 11;
		case 24:
			return 12;
		case 32:
			return 13;
		case 48:
			return 14;
		case 64:
			return 15;
		default:
			return 255;
	}
}

inline uint8_t MCP2517_C::Get_Data_Length(uint8_t DLC){
	switch(DLC){
		case 1:
			return 1;
		case 2:
			return 2;
		case 3:
			return 3;
		case 4:
			return 4;
		case 5:
			return 5;
		case 6:
			return 6;
		case 7:
			return 7;
		case 8:
			return 8;
		case 9:
			return 12;
		case 10:
			return 16;
		case 11:
			return 20;
		case 12:
			return 24;
		case 13:
			return 32;
		case 14:
			return 48;
		case 15:
			return 64;
		default:
			return 0;
	}
}

inline uint16_t MCP2517_C::Get_FIFOCON_Addr(uint8_t fifoNum){
	return ((uint16_t) ADDR_E::C1TXQCON + (0xc * fifoNum));
}

inline uint16_t MCP2517_C::Get_FIFOSTA_Addr(uint8_t fifoNum){
	return ((uint16_t) ADDR_E::C1TXQSTA + (0xc * fifoNum));
}

inline uint16_t MCP2517_C::Get_FIFOUA_Addr(uint8_t fifoNum){
	return ((uint16_t) ADDR_E::C1TXQUA + (0xc * fifoNum));
}

#warning Temporary CANID
void MCP2517_C::Generate_CAN_ID(){
	CANID = 69;
}

// Reset the CAN controller
void MCP2517_C::Reset(){
	char temp[4] = {0,0,0,0};
	while(Get_Status() != Idle);
	Select_Slave(true);
	Transfer_Blocking(temp, 4);
	while(Get_Status() != Idle);
	Select_Slave(false);
}

// Sends a number of bytes to the specified address of the MCP2517
uint8_t MCP2517_C::Send_Buffer(enum ADDR_E addr, char* data, uint8_t length){
	if (Get_Status() == Idle){
		// Write buffer
		msgBuff[0] = ((char) MCP2517_INSTR_E::Write << 4) | ((uint16_t) addr >> 8);
		msgBuff[1] = (uint8_t) addr & 0xff;
		
		for (uint8_t i = 0; i < length; i++){
			msgBuff[i+2] = data[i];
		}
		
		Select_Slave(true);
		Send(length + 2);
		
		// Return success
		return 1;
	} else {
		// Return not success
		return 0;
	}
};

// Reads a number of bytes from the specified address of the MCP2517
uint8_t MCP2517_C::Receive_Buffer(enum ADDR_E addr, uint8_t length){
	if (Get_Status() == Idle){
		// Write buffer
		msgBuff[0] = ((char) MCP2517_INSTR_E::Read << 4) | ((uint16_t) addr >> 8);
		msgBuff[1] = (uint8_t) addr & 0xff;
		
		Select_Slave(true);
		Receive(length + 2);
		
		return 1;
	} else {
		return 0;
	}
};

// Write a word to CAN controller without interrupts.
void MCP2517_C::Write_Word_Blocking(enum ADDR_E addr, uint32_t data){
	char temp[6];
	temp[0] = ((char) MCP2517_INSTR_E::Write << 4) | ((uint16_t) addr >> 8);
	temp[1] = (uint8_t) addr & 0xff;
	temp[2] = data & 0xff;
	temp[3] = (data >> 8) & 0xff;
	temp[4] = (data >> 16) & 0xff;
	temp[5] = (data >> 24) & 0xff;
	while(Get_Status() != Idle);
	Select_Slave(true);
	Transfer_Blocking(temp, 6);
	Select_Slave(false);
}

uint32_t MCP2517_C::Receive_Word_Blocking(enum ADDR_E addr){
	char temp[10];
	temp[0] = ((char) MCP2517_INSTR_E::Read << 4) | ((uint16_t) addr >> 8);
	temp[1] = (uint8_t) addr & 0xff;
	while(Get_Status() != Idle);
	Select_Slave(true);
	Transfer_Blocking(temp, 10);
	Select_Slave(false);
	uint32_t result = temp[2] | (temp[3] << 8) | (temp[4] << 16) | (temp[5] << 24);
	return result;
}

void MCP2517_C::FIFO_Increment(uint8_t fifoNum, uint8_t txRequest){
	uint16_t addr = Get_FIFOCON_Addr(fifoNum) + 1;
		
	char data = 1 | (txRequest << 1);
	
	Send_Buffer((ADDR_E) addr, &data, 1);
}

void MCP2517_C::FIFO_User_Address(uint8_t fifoNum){
	uint16_t addr = Get_FIFOUA_Addr(fifoNum);
	
	Receive_Buffer((ADDR_E) addr, 2);
}

// Attempts to start a message transfer
uint8_t MCP2517_C::Transmit_Message(char* data, uint8_t length, bool broadcast){
	if (msgState == Msg_Idle){
		for (uint8_t i = 0; i < length; i++){
			msgBuff[i+10] = data[i];
		}
	
		payload = length;
		
		msgState = Msg_Tx_Addr;
		broadcasting = broadcast;
		FIFO_User_Address(2);
		return 1;
	} else {
		return 0;
	}
}

// Send the payload
// Payload was loaded to buffer by Transmit_Message
void MCP2517_C::Send_Message_Object(uint16_t addr){
	msgBuff[0] = ((char) MCP2517_INSTR_E::Write << 4) | ((uint16_t) addr >> 8);
	msgBuff[1] = addr & 0xff;
		
	msgBuff[2] = CANID & 0xff;
	msgBuff[3] = ((CANID >> 8) & 0b0011) | (!broadcasting ? (1 << 2) : 0);
	msgBuff[4] = 0;
	msgBuff[5] = 0;
		
	msgBuff[6] = (0b1000 << 4) | Get_DLC(payload);
	msgBuff[7] = 0;
	msgBuff[8] = 0;
	msgBuff[9] = 0;
		
	Select_Slave(true);
	Send(payload + 10);
}

// Request status info of FIFO
void MCP2517_C::FIFO_Status(uint8_t fifoNum){
	uint16_t addr = Get_FIFOSTA_Addr(fifoNum);
	
	Receive_Buffer((ADDR_E) addr, 1);
}

// Checks MCP2517 interrupt pin
void MCP2517_C::Check_Rx_Int(){
	if (!port_pin_get_input_level(interruptPin)){
		msgState = Msg_Rx_Addr;
		FIFO_User_Address(1);
	}
}

// Check MCP2517 interrupt register every RTC tick
void MCP2517_C::Check_Rx_RTC(){
	static uint32_t count = 0;
	if (count < RTC->MODE0.COUNT.reg){
		msgState = Msg_Rx_Int;
		count = RTC->MODE0.COUNT.reg;
		FIFO_Status(1);
	}
}

void MCP2517_C::State_Machine(){
	switch(msgState){
		case Msg_Idle:
			if (interruptPin < 64){
				Check_Rx_Int();
			} else {
				Check_Rx_RTC();
			}
			break;
		case Msg_Rx_Int:
			if (Get_Status() == Rx_Ready){
				char temp[3];
				Read_Buffer(temp);
				if (temp[2] & 0x01){
					// Message waiting in buffer
					msgState = Msg_Rx_Addr;
					FIFO_User_Address(1);
				} else {
					// No message
					msgState = Msg_Idle;
				}
			}
			break;
		case Msg_Rx_Addr:
			if (Get_Status() == Rx_Ready){
				msgState = Msg_Rx_Data;
				char temp[4];
				Read_Buffer(temp);
				
				uint16_t addr = (temp[3] << 8) | temp[2];
				addr += (uint16_t) ADDR_E::RAM_START;
				Receive_Buffer((ADDR_E) addr, 26);
			}
			break;
		case Msg_Rx_Data:
			if (Get_Status() == Rx_Ready){
				char buffer[26];
				Read_Buffer(buffer);
				
				Rx_Callback(&buffer[10]);
				msgState = Msg_Rx_FIFO;
				FIFO_Increment(1, 0);
			}
			break;
		case Msg_Rx_FIFO:
			if (Get_Status() == Idle){
				msgState = Msg_Idle;
			}
			break;
		case Msg_Tx_Addr:
			if (Get_Status() == Rx_Ready){
				msgState = Msg_Tx_Data;
				char temp[4];
				Read_Buffer(temp);
				
				uint16_t addr = (temp[3] << 8) | temp[2];
				addr += (uint16_t) ADDR_E::RAM_START;
				Send_Message_Object(addr);
			}
			break;
		case Msg_Tx_Data:
			if (Get_Status() == Idle){
				msgState = Msg_Tx_FIFO;
				FIFO_Increment(2, 1);
			}
			break;
		case Msg_Tx_FIFO:
			if (Get_Status() == Idle){
				msgState = Msg_Idle;
			}
			break;
		default:
			break;
	}
}

// Interrupt handler for CAN controller
inline void MCP2517_C::Handler(){
	if (com->SPI.INTFLAG.bit.TXC && com->SPI.INTENSET.bit.TXC) {
		com->SPI.INTENCLR.reg = SERCOM_SPI_INTENCLR_TXC;
		// End transmission
		Select_Slave(false);
		currentState = Idle;
	}
	if (com->SPI.INTFLAG.bit.DRE && com->SPI.INTENSET.bit.DRE) {
		// Data register empty
		if (currentState == Tx) {
			com->SPI.DATA.reg = msgBuff[txIndex];
		} else {
			if (txIndex <= 2){
				// Send instruction and address bytes
				com->SPI.DATA.reg = msgBuff[txIndex];
			} else {
				// Send dummy byte
				com->SPI.DATA.reg = 0;		
			}
			
		}
		
		txIndex++;
		
		if (txIndex >= msgLength) {
			com->SPI.INTENCLR.reg = SERCOM_SPI_INTENCLR_DRE;
			if (currentState == Tx) {
				com->SPI.INTENSET.reg = SERCOM_SPI_INTENSET_TXC;	// Wait for transmission complete
				com->SPI.INTFLAG.reg = SERCOM_SPI_INTENSET_TXC;		// Clear flag
			}
		}
	}
	if (com->SPI.INTFLAG.bit.RXC && com->SPI.INTENSET.bit.RXC) {
		if (currentState == Rx) {
			msgBuff[rxIndex++] = com->SPI.DATA.reg;
			if (rxIndex >= msgLength) {
				Select_Slave(false);
				currentState = Rx_Ready;
			}
		} else {
			volatile uint8_t dumdum = com->SPI.DATA.reg;
		}
	}
	
	//NVIC_ClearPendingIRQ(SERCOM5_IRQn);
}

#endif /* MCP2517_H_ */