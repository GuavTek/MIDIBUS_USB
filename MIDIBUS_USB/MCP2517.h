﻿/*
 * MCP2517.h
 *
 * Created: 01/07/2021 17:36:38
 *  Author: GuavTek
 */ 


#ifndef MCP2517_H_
#define MCP2517_H_

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

enum class MCP2517_ADDR_E {
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
	RAM_START		= 0x400,
	RAM_END			= 0xbff,
	OSC				= 0xe00,
	IOCON			= 0xe04,
	CRC				= 0xe08,
	ECCCON			= 0xe0c,
	ECCSTAT			= 0xe10
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
		void Write_Word_Blocking(enum MCP2517_ADDR_E addr, uint32_t data);
		uint8_t Send_Buffer(enum MCP2517_ADDR_E addr, char* data, uint8_t length);
		uint8_t Receive_Buffer(enum MCP2517_ADDR_E addr, uint8_t length);
		void Init();
		static uint8_t Get_DLC(uint8_t dataLength);
		static uint8_t Get_Data_Length(uint8_t DLC);
		inline void Handler();
	protected:
		void Reset();
		void Generate_CAN_ID();
		uint32_t CANID;
};

void MCP2517_C::Init(){
	Reset();
	uint32_t temp = 0;
	
	temp = 0;
	temp |= 0b010 << 29;	// Payload size = 16 bytes
	temp |= 0b00111 << 24;	// FIFO size = 8 messages
	temp |= 0b11 << 21;		// Unlimited retransmission attempts
	Write_Word_Blocking(MCP2517_ADDR_E::C1TXQCON, temp);
	
	temp = 0;
	//temp |= 1 << 20;		// Tx event interrupt enable, Flag is in position 4
	temp |= 1 << 17;		// Rx interrupt enable, Flag is in position 1
	Write_Word_Blocking(MCP2517_ADDR_E::C1INT, temp);
	
	temp = 0;
	temp |= 1 << 25;		// Enable edge filtering
	temp |= 0b11 << 16;		// Auto delay compensation
	Write_Word_Blocking(MCP2517_ADDR_E::C1TDC, temp);
	
	temp = 0;
	temp |= 0x01 << 24;		// Time quanta prescaler Tq = 2/20MHz = 100ns
	temp |= 0x05 << 16;		// Time segment 1 = 6 Tq
	temp |= 0x02 << 8;		// Time segment 2 = 3 Tq
	temp |= 0x00;			// Sync jump width = 1 Tq
	Write_Word_Blocking(MCP2517_ADDR_E::C1NBTCFG, temp);
	Write_Word_Blocking(MCP2517_ADDR_E::C1DBTCFG, temp);
	
	temp = 0;
	temp |= 0b0010 << 28;	// 4 cycle delay between transmissions
	temp |= 0b000 << 24;	// Normal CAN FD mode
	temp |= 1 << 12;		// Disable bit-rate switching
	temp |= 1 << 5;			// Use non-zero initial crc vector
	Write_Word_Blocking(MCP2517_ADDR_E::C1CON, temp);
}

static uint8_t MCP2517_C::Get_DLC(uint8_t dataLength){
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

static uint8_t MCP2517_C::Get_Data_Length(uint8_t DLC){
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


// Reset the CAN controller
void MCP2517_C::Reset(){
	char temp[2] = {0,0};
	while(Get_Status());
	Select_Slave(true);
	Send(&temp, 2);
	while(Get_Status());
	Select_Slave(false);
}

// Sends a number of bytes to the specified address of the MCP2517
uint8_t MCP2517_C::Send_Buffer(enum MCP2517_ADDR_E addr, char* data, uint8_t length){
	if (Get_Status() == Idle){
		// Write buffer
		msgBuff[0] = ((char) MCP2517_INSTR_E::Write << 4) | ((char) addr >> 8);
		msgBuff[1] = addr & 0xff;
		
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
uint8_t MCP2517_C::Receive_Buffer(enum MCP2517_ADDR_E addr, uint8_t length){
	if (Get_Status() == Idle){
		// Write buffer
		msgBuff[0] = ((char) MCP2517_INSTR_E::Read << 4) | ((char) addr >> 8);
		msgBuff[1] = addr & 0xff;
		
		Select_Slave(true);
		Receive(length + 2);
		
		return 1;
	} else {
		return 0;
	}
};

// Write a word to CAN controller and block until done.
void MCP2517_C::Write_Word_Blocking(enum MCP2517_ADDR_E addr, uint32_t data){
	char temp[6];
	temp[0] = ((char) MCP2517_INSTR_E::Write << 4) | ((char) addr >> 8);
	temp[1] = addr & 0xff;
	temp[2] = data & 0xff;
	temp[3] = (data >> 8) & 0xff;
	temp[4] = (data >> 16) & 0xff;
	temp[5] = (data >> 24) & 0xff;
	while(Get_Status() != Idle);
	Select_Slave(true);
	Send_Blocking(&temp, 6);
	Select_Slave(false);
}

// Interrupt handler for CAN controller
inline void MCP2517_C::Handler(){
	if (com->SPI.INTFLAG.bit.DRE && com->SPI.INTENSET.bit.DRE) {
		// Data register empty
		if (currentState == Tx) {
			com->SPI.DATA.reg = msgBuff[txIndex];
		}
		
		txIndex++;
		
		if (txIndex >= msgLength) {
			com->SPI.INTENCLR.reg = SERCOM_SPI_INTENCLR_DRE;
			if (currentState == Tx) {
				Select_Slave(false);
				currentState = Idle;
			}
		} else if(currentState == Rx) {
			if (txIndex <= 2){
				// Send instruction and address bytes
				com->SPI.DATA.reg = msgBuff[txIndex-1];
			} else {
				// Send dummy byte
				com->SPI.DATA.reg = 0;		
			}
			
		}
	}
	if (com->SPI.INTFLAG.bit.RXC && com->SPI.INTENSET.bit.RXC) {
		if (currentState == Rx) {
			msgBuff[rxIndex] = com->SPI.DATA.reg;
		}
		
		rxIndex++;
		
		if (rxIndex >= msgLength) {
			com->SPI.INTENCLR.reg = SERCOM_SPI_INTENCLR_RXC;
			if (currentState == Rx) {
				Select_Slave(false);
				currentState = Rx_Ready;
			}
		}
	}
	
	//NVIC_ClearPendingIRQ(SERCOM5_IRQn);
}

#endif /* MCP2517_H_ */