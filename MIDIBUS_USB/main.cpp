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
#include "SPI.h"
#include "MIDI_Driver.h"
#include "tusb.h"


MCP2517_C CAN(SERCOM5);

void CAN_Receive(CAN_Rx_msg_t* data);
void USB_Init();
void midi_task(void);

void MIDI2_CAN_voice_handler (struct MIDI2_voice_t* msg);
void MIDI2_CAN_data128_handler (struct MIDI2_data128_t* msg);
void MIDI2_CAN_data64_handler (struct MIDI2_data64_t* msg);
void MIDI2_CAN_com_handler (struct MIDI2_com_t* msg);
void MIDI2_CAN_util_handler (struct MIDI2_util_t* msg);
void MIDI1_CAN_handler (struct MIDI1_msg_t* msg);

void MIDI2_USB_voice_handler (struct MIDI2_voice_t* msg);
void MIDI2_USB_data128_handler (struct MIDI2_data128_t* msg);
void MIDI2_USB_data64_handler (struct MIDI2_data64_t* msg);
void MIDI2_USB_com_handler (struct MIDI2_com_t* msg);
void MIDI2_USB_util_handler (struct MIDI2_util_t* msg);
void MIDI1_USB_handler (struct MIDI1_msg_t* msg);

MIDI_C MIDI_USB(1);
MIDI_C MIDI_CAN(2);

uint32_t midiID = 616;
uint32_t blinkTime = 100;
uint32_t blinkTime2 = 100;
volatile uint32_t system_ticks = 0;

int main(void)
{
	system_init();
	CAN.Init(CAN_CONF, SPI_CONF);
	CAN.Set_Rx_Callback(CAN_Receive);
	
	PORT->Group[0].DIRSET.reg = (1 << 16) | (1 << 17);
	PORT->Group[0].OUTCLR.reg = 1 << 16;

	// Enable USB ID pin
	PORT->Group[0].DIRCLR.reg = (1 << 27);
	PORT->Group[0].PINCFG[27].bit.INEN = 1;
	
	USB_Init();
	tusb_init();

	MIDI_CAN.Set_handler(MIDI2_CAN_voice_handler);
	MIDI_CAN.Set_handler(MIDI2_CAN_data128_handler);
	MIDI_CAN.Set_handler(MIDI2_CAN_data64_handler);
	MIDI_CAN.Set_handler(MIDI2_CAN_com_handler);
	MIDI_CAN.Set_handler(MIDI2_CAN_util_handler);
	MIDI_CAN.Set_handler(MIDI1_CAN_handler);
	
	MIDI_USB.Set_handler(MIDI2_USB_voice_handler);
	MIDI_USB.Set_handler(MIDI2_USB_data128_handler);
	MIDI_USB.Set_handler(MIDI2_USB_data64_handler);
	MIDI_USB.Set_handler(MIDI2_USB_com_handler);
	MIDI_USB.Set_handler(MIDI2_USB_util_handler);
	MIDI_USB.Set_handler(MIDI1_USB_handler);
	/**/
	
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
	//CAN.Transmit_Message(&message, 2);
	
    while (1) 
    {
		CAN.State_Machine();
		
		// USB tasks
		tud_task();
		//tuh_task();
		
		midi_task();
		
		static uint32_t timrr = 0;
		if (timrr < system_ticks/**/)	{
			timrr = system_ticks + blinkTime;
			PORT->Group[0].OUTTGL.reg = 1 << 17;
			
		}
    }
}

// Initialize clocks and pins for the USB port
void USB_Init(){
	
	NVMCTRL->CTRLB.reg |= NVMCTRL_CTRLB_RWS(2);
	
	SysTick_Config(F_CPU/1000);
	
	/* USB Clock init
	 * The USB module requires a GCLK_USB of 48 MHz ~ 0.25% clock
	 * for low speed and full speed operation. */
	
	// Enable USB clock
	PM->APBCMASK.reg |= PM_APBBMASK_USB;
	PM->AHBMASK.reg |= PM_AHBMASK_USB;
	
	// Select generic clock
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | (GCLK_CLKCTRL_ID_USB);

	// USB Pin Init
	PORT->Group[0].DIRSET.reg = (1 << 24) | (1 << 25);
	PORT->Group[0].OUTCLR.reg = (1 << 24) | (1 << 25);
	PORT->Group[0].PINCFG[24].bit.PULLEN = 0;
	PORT->Group[0].PINCFG[25].bit.PULLEN = 0;
	
	pin_set_peripheral_function(PINMUX_PA24G_USB_DM);
	pin_set_peripheral_function(PINMUX_PA25G_USB_DP);
	
}

// Handle MIDI CAN data
void CAN_Receive(CAN_Rx_msg_t* data){
	MIDI_CAN.Decode(data->payload, CAN.Get_Data_Length(data->dataLengthCode) );
}


void MIDI2_CAN_voice_handler (struct MIDI2_voice_t* msg){
	if (MIDI_USB.Get_Version() == 1){
		// Voice packages are MIDI 2.0 only?
		return;
	}
	
	char tempData[8];
	uint8_t length;
	length = MIDI_USB.Encode(tempData, msg);
	tud_midi_stream_write(0, (uint8_t*)(tempData), length);
}

void MIDI2_CAN_data128_handler (struct MIDI2_data128_t* msg){
	if (MIDI_USB.Get_Version() == 1){
		// 128 bit sysex messages are MIDI 2.0 only
		return;
	}
	
	char tempData[16];
	uint8_t length;
	length = MIDI_USB.Encode(tempData, msg);
	tud_midi_stream_write(0, (uint8_t*)(tempData), length);
}

void MIDI2_CAN_data64_handler (struct MIDI2_data64_t* msg){
	char tempData[8];
	uint8_t length;
	length = MIDI_USB.Encode(tempData, msg, MIDI_USB.Get_Version());
	tud_midi_stream_write(0, (uint8_t*)(tempData), length);
}

void MIDI2_CAN_com_handler (struct MIDI2_com_t* msg){
	char tempData[4];
	uint8_t length;
	length = MIDI_USB.Encode(tempData, msg, MIDI_USB.Get_Version());
	tud_midi_stream_write(0, (uint8_t*)(tempData), length);
}

void MIDI2_CAN_util_handler (struct MIDI2_util_t* msg){
	if (MIDI_USB.Get_Version() == 1){
		// Utility messages are MIDI 2.0 only
		return;
	}
	
	char tempData[4];
	uint8_t length;
	length = MIDI_USB.Encode(tempData, msg);
	tud_midi_stream_write(0, (uint8_t*)(tempData), length);
}

void MIDI1_CAN_handler (struct MIDI1_msg_t* msg){
	char tempData[4];
	uint8_t length;
	length = MIDI_USB.Encode(tempData, msg, MIDI_USB.Get_Version());
	tud_midi_stream_write(0, (uint8_t*)(tempData), length);
	//PORT->Group[0].OUTSET.reg = 1 << 16;
}

// Handle USB midi data
void MIDI2_USB_voice_handler (struct MIDI2_voice_t* msg){
	if (MIDI_USB.Get_Version() == 1){
		return;
	}
	char tempData[8];
	
}

void MIDI2_USB_data128_handler (struct MIDI2_data128_t* msg){
	if (MIDI_USB.Get_Version() == 1){
		return;
	}
	char tempData[16];
	
}

void MIDI2_USB_data64_handler (struct MIDI2_data64_t* msg){
	if (MIDI_USB.Get_Version() == 1){
		return;
	}
	char tempData[8];
	
}

void MIDI2_USB_com_handler (struct MIDI2_com_t* msg){
	if (MIDI_USB.Get_Version() == 1){
		return;
	}
	char tempData[4];
	
}

void MIDI2_USB_util_handler (struct MIDI2_util_t* msg){
	if (MIDI_USB.Get_Version() == 1){
		return;
	}
	char tempData[4];
	uint8_t length;
	length = MIDI_CAN.Encode(tempData, msg);
	CAN_Tx_msg_t txMsg;
	txMsg.dataLengthCode = CAN.Get_DLC(length);
	txMsg.payload = tempData;
	txMsg.id = midiID;
	CAN.Transmit_Message(&txMsg, 2);
}

void MIDI1_USB_handler (struct MIDI1_msg_t* msg){
	char tempData[4];
	uint8_t length;
	length = MIDI_CAN.Encode(tempData, msg, 2);
	CAN_Tx_msg_t txMsg;
	txMsg.dataLengthCode = CAN.Get_DLC(length);
	txMsg.payload = tempData;
	txMsg.id = midiID;
	CAN.Transmit_Message(&txMsg, 2);
}

void midi_task(void)
{
	uint8_t packet[16];
	uint8_t length;
	while ( tud_midi_available() ) {
		length = tud_midi_stream_read(packet, 16);
		MIDI_USB.Decode((char*)(packet), length);
	}
}
/**/

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
	blinkTime = 2000;
	blinkTime2 = 1998;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
	blinkTime = 50;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
	(void) remote_wakeup_en;
	//blinkTime = 100;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
	blinkTime = 1000;
}

void SysTick_Handler (void)
{
	system_ticks++;
}

uint32_t board_millis(void)
{
	return system_ticks;
}

	}

void USB_Handler(void){
	tud_int_handler(0);
	
	//tuh_int_handler(0);
}

// For some reason the SERCOM5 interrupt leads to the SERCOM3 handler
// That was a painful debugging session
void SERCOM5_Handler(void){
	CAN.Handler();
}