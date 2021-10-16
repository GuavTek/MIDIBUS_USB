/*
 * MIDIBUS_USB.cpp
 *
 * Created: 26/06/2021 19:48:09
 * Author : GuavTek
 */ 

#include "MIDI_Config.h"
#include "sam.h"
#include "asf.h"
#include "MCP2517.h"
#include "MCP2517.cpp"
#include "SPI.h"
#include "SPI.cpp"
#include "MIDI_Driver.h"


MCP2517_C CAN(SERCOM5);

void CAN_Receive(CAN_Rx_msg_t* data);

//MIDI_C MIDI_USB(2);
//MIDI_C MIDI_CAN(2);

int main(void)
{
	system_init();
	CAN.Init(CAN_CONF, SPI_CONF);
	CAN.Set_Rx_Callback(CAN_Receive);
	
	PORT->Group[0].DIRSET.reg = (1 << 16) | (1 << 17);
//	PORT->Group[0].OUTSET.reg = 1 << 16;
	
	NVIC_EnableIRQ(SERCOM5_IRQn);
	system_interrupt_enable_global();
	
	char temp[4];
	CAN_Tx_msg_t message;
	message.payload = temp;
	message.canFDFrame = true;
	message.dataLengthCode = CAN.Get_DLC(4);
	message.id = 69;
	message.extendedID = false;
	temp[2] = 70;
	CAN.Transmit_Message(&message, 2);
	
    while (1) 
    {
		CAN.State_Machine();
    }
}

void CAN_Receive(CAN_Rx_msg_t* data){
	PORT->Group[0].OUTSET.reg = 1 << 16;
	if (data->payload[2] == 70){
		PORT->Group[0].OUTSET.reg = 1 << 17;
	}
}

// For some reason the SERCOM5 interrupt leads to the SERCOM3 handler
// That was a painful debugging session
void SERCOM5_Handler(void){
	CAN.Handler();
}