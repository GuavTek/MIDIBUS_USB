/*
 * MIDIBUS_USB.cpp
 *
 * Created: 26/06/2021 19:48:09
 * Author : GuavTek
 */ 



#include "sam.h"
#include "asf.h"
#include "MIDI_Driver.h"
#include "MCP2517.h"
#include "SPI.h"

const spi_config_t SPI_CONF = {
	.sercomNum = 5,
	.dipoVal = 0x3,
	.dopoVal = 0x0,
	.speed = 8000000,
	.pin_cs = PIN_PB22,
	.pinmux_mosi = PINMUX_PB02D_SERCOM5_PAD0,
	.pinmux_miso = PINMUX_PB23D_SERCOM5_PAD3,
	.pinmux_sck = PINMUX_PB03D_SERCOM5_PAD1
};

MCP2517_C CAN(SERCOM5);

void CAN_Receive(char* data);

//MIDI_C MIDI_USB(2);
//MIDI_C MIDI_CAN(2);

int main(void)
{
	system_init();
	CAN.Init(0, SPI_CONF);
	CAN.Set_Rx_Callback(CAN_Receive);
	
	PORT->Group[0].DIRSET.reg = (1 << 16) | (1 << 17);
	PORT->Group[0].OUTSET.reg = 1 << 16;
	
	NVIC_EnableIRQ(SERCOM5_IRQn);
	system_interrupt_enable_global();
	
	char temp[4];
	temp[2] = 69;
	CAN.Transmit_Message(temp, 4, 1);
	
    while (1) 
    {
		CAN.State_Machine();
    }
}

void CAN_Receive(char* data){
	if (data[2] == 69){
		PORT->Group[0].OUTSET.reg = 1 << 17;
	}
}

// For some reason the SERCOM5 interrupt leads to the SERCOM3 handler
// That was a painful debugging session
void SERCOM3_Handler(void){
	CAN.Handler();
}