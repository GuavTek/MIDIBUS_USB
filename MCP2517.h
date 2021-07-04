/*
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

class MCP2517_C : SPI_C {
	public:
		void Write_Word_Blocking(enum MCP2517_ADDR_E addr, uint32_t data);
		void Init();
		
	protected:
		void Reset();
};

void MCP2517_C::Init(){
	Reset();
	uint32_t temp = 0;
	
	temp = 0;
	temp |= 0b0010 << 28;	// 4 cycle delay between transmissions
	temp |= 0b000 << 24;	// Normal CAN FD mode
	temp |= 1 << 12;		// Disable bit-rate switching
	temp |= 1 << 5;			// Use non-zero initial crc vector
	Write_Word_Blocking(MCP2517_ADDR_E::C1CON, temp);
}

void MCP2517_C::Reset(){
	char temp[2] = {0,0};
	while(Get_Status());
	Select_Slave(true);
	Send(&temp, 2);
	while(Get_Status());
	Select_Slave(false);
}

void MCP2517_C::Write_Word_Blocking(enum MCP2517_ADDR_E addr, uint32_t data){
	char temp[6];
	temp[0] = ((char) MCP2517_INSTR_E::Write << 4) | ((char) addr >> 8);
	temp[1] = addr & 0xff;
	temp[2] = data & 0xff;
	temp[3] = (data >> 8) & 0xff;
	temp[4] = (data >> 16) & 0xff;
	temp[5] = (data >> 24) & 0xff;
	while(Get_Status());
	Select_Slave(true);
	Send(&temp, 6);
	while(Get_Status());
	Select_Slave(false);
}

#endif /* MCP2517_H_ */