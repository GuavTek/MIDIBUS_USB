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
#include "DMA_driver.h"
#include "I2S_driver.h"
//#include "Audio_driver.h"
#include "tusb.h"


MCP2517_C CAN(SERCOM5);

void CAN_Receive(CAN_Rx_msg_t* data);
void USB_Init();
void audio_task(void);
void midi_task(void);
void audio_dma_init();

void MIDI_CAN_UMP_handler(struct MIDI_UMP_t* msg);
void MIDI_USB_UMP_handler(struct MIDI_UMP_t* msg);

MIDI_C MIDI_USB(1);
MIDI_C MIDI_CAN(2);

uint32_t midiID = 616;
uint32_t blinkTime = 100;
uint32_t blinkTime2 = 100;
volatile uint32_t system_ticks = 0;

// For debugging
volatile Dmac* tempDMA = DMAC;
volatile uint64_t dropped_bytes = 0;
volatile I2s* tempI2S = I2S;
volatile Usb* tempUSB = USB;
volatile uint64_t txufl_count = 0;
volatile uint64_t rxofl_count = 0;
volatile uint64_t txufr_count = 0;
volatile uint64_t rxofr_count = 0;
volatile uint64_t terr_count = 0;

/*
inline void Debug_func() {
	if ((I2S->INTFLAG.bit.RXOR0) && mic_active && (i2s_rx_descriptor_wb->btctrl.valid == 0)){
		rxofl_count++;
	}
	if ((I2S->INTFLAG.bit.RXOR1) && spk_active && (i2s_tx_descriptor_wb->btctrl.valid == 0)){
		txufl_count++;
	}
	if (DMAC->INTSTATUS.bit.CHINT2){
		terr_count++;
	}
};
/**/
void hid_app_task();

int main(void)
{
	system_init();
	CAN.Init(CAN_CONF, SPI_CONF);
	CAN.Set_Rx_Callback(CAN_Receive);
	//i2s_init(44100);
	
	dma_init(base_descriptor, wrback_descriptor);
	//audio_dma_init();
	
	PORT->Group[0].DIRSET.reg = (1 << 16) | (1 << 17);
	PORT->Group[0].OUTCLR.reg = 1 << 16;

	
	USB_Init();
	//tusb_init();
	tuh_init(0);

	MIDI_CAN.Set_handler(MIDI_CAN_UMP_handler);
	MIDI_USB.Set_handler(MIDI_USB_UMP_handler);
	
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
		//tud_task();
		tuh_task();
		hid_app_task();
		//Debug_func();
		
		//audio_task();
		//midi_task();
		
		/*
		if (PORT->Group[0].IN.reg & (1 << 27)){
			PORT->Group[0].OUTCLR.reg = 1 << 16;
		} else {
			PORT->Group[0].OUTSET.reg = 1 << 16;
		} /**/
			
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
	
	// Enable USB ID pin
	PORT->Group[0].DIRCLR.reg = (1 << 27);
	PORT->Group[0].PINCFG[27].bit.INEN = 1;
}

// Handle MIDI CAN data
void CAN_Receive(CAN_Rx_msg_t* data){
	MIDI_CAN.Decode(data->payload, CAN.Get_Data_Length(data->dataLengthCode) );
}

void MIDI_CAN_UMP_handler(struct MIDI_UMP_t* msg){
	char tempData[16];
	uint8_t length;
	length = MIDI_USB.Encode(tempData, msg, MIDI_USB.Get_Version());
	
	if (length > 0){
		//tud_midi_stream_write(0, (uint8_t*)(tempData), length);
	}
}

// Handle USB midi data
void MIDI_USB_UMP_handler(struct MIDI_UMP_t* msg){
	char tempData[16];
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
	//while ( tud_midi_available() ) {
	//	length = tud_midi_stream_read(packet, 16);
		MIDI_USB.Decode((char*)(packet), length);
	//}
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

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
	(void) remote_wakeup_en;
	blinkTime = 100;
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

void DMAC_Handler(){
	uint32_t tempFlag = DMAC->INTPEND.reg;
	tempFlag &= DMAC_INTPEND_ID_Msk;
	
	dma_resume(tempFlag);
	DMAC->CHID.reg = tempFlag;
	DMAC->CHINTENCLR.bit.SUSP = 1;
}

void USB_Handler(void){
	//tud_int_handler(0);
	
	tuh_int_handler(0);
}

// For some reason the SERCOM5 interrupt leads to the SERCOM3 handler
// That was a painful debugging session
void SERCOM5_Handler(void){
	CAN.Handler();
}


/* Testing temporary */

/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021, Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb.h"

/* From https://www.kernel.org/doc/html/latest/input/gamepad.html
          ____________________________              __
         / [__ZL__]          [__ZR__] \               |
        / [__ TL __]        [__ TR __] \              | Front Triggers
     __/________________________________\__         __|
    /                                  _   \          |
   /      /\           __             (N)   \         |
  /       ||      __  |MO|  __     _       _ \        | Main Pad
 |    <===DP===> |SE|      |ST|   (W) -|- (E) |       |
  \       ||    ___          ___       _     /        |
  /\      \/   /   \        /   \     (S)   /\      __|
 /  \________ | LS  | ____ |  RS | ________/  \       |
|         /  \ \___/ /    \ \___/ /  \         |      | Control Sticks
|        /    \_____/      \_____/    \        |    __|
|       /                              \       |
 \_____/                                \_____/

     |________|______|    |______|___________|
       D-Pad    Left       Right   Action Pad
               Stick       Stick

                 |_____________|
                    Menu Pad

  Most gamepads have the following features:
  - Action-Pad 4 buttons in diamonds-shape (on the right side) NORTH, SOUTH, WEST and EAST.
  - D-Pad (Direction-pad) 4 buttons (on the left side) that point up, down, left and right.
  - Menu-Pad Different constellations, but most-times 2 buttons: SELECT - START.
  - Analog-Sticks provide freely moveable sticks to control directions, Analog-sticks may also
  provide a digital button if you press them.
  - Triggers are located on the upper-side of the pad in vertical direction. The upper buttons
  are normally named Left- and Right-Triggers, the lower buttons Z-Left and Z-Right.
  - Rumble Many devices provide force-feedback features. But are mostly just simple rumble motors.
 */

// Sony DS4 report layout detail https://www.psdevwiki.com/ps4/DS4-USB
typedef struct TU_ATTR_PACKED
{
  uint8_t x, y, z, rz; // joystick

  struct {
    uint8_t dpad     : 4; // (hat format, 0x08 is released, 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW)
    uint8_t square   : 1; // west
    uint8_t cross    : 1; // south
    uint8_t circle   : 1; // east
    uint8_t triangle : 1; // north
  };

  struct {
    uint8_t l1     : 1;
    uint8_t r1     : 1;
    uint8_t l2     : 1;
    uint8_t r2     : 1;
    uint8_t share  : 1;
    uint8_t option : 1;
    uint8_t l3     : 1;
    uint8_t r3     : 1;
  };

  struct {
    uint8_t ps      : 1; // playstation button
    uint8_t tpad    : 1; // track pad click
    uint8_t counter : 6; // +1 each report
  };

  // comment out since not used by this example
  // uint8_t l2_trigger; // 0 released, 0xff fully pressed
  // uint8_t r2_trigger; // as above

  //  uint16_t timestamp;
  //  uint8_t  battery;
  //
  //  int16_t gyro[3];  // x, y, z;
  //  int16_t accel[3]; // x, y, z

  // there is still lots more info

} sony_ds4_report_t;

// check if device is Sony DualShock 4
static inline bool is_sony_ds4(uint8_t dev_addr)
{
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  return ( (vid == 0x054c && (pid == 0x09cc || pid == 0x05c4)) // Sony DualShock4 
           || (vid == 0x0f0d && pid == 0x005e)                 // Hori FC4 
           || (vid == 0x0f0d && pid == 0x00ee)                 // Hori PS4 Mini (PS4-099U) 
           || (vid == 0x1f4f && pid == 0x1002)                 // ASW GG xrd controller
         );
}

//--------------------------------------------------------------------+
// MACRO TYPEDEF CONSTANT ENUM DECLARATION
//--------------------------------------------------------------------+

void hid_app_task(void)
{
  // nothing to do
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
  (void)desc_report;
  (void)desc_len;
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

	blinkTime = 1000;
  //printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);
  //printf("VID = %04x, PID = %04x\r\n", vid, pid);

  // Sony DualShock 4 [CUH-ZCT2x]
  if ( is_sony_ds4(dev_addr) )
  {
    // request to receive report
    // tuh_hid_report_received_cb() will be invoked when report is available
    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      //printf("Error: cannot request to receive report\r\n");
    }
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  //printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
	blinkTime = 100;
}

// check if different than 2
bool diff_than_2(uint8_t x, uint8_t y)
{
  return (x - y > 2) || (y - x > 2);
}

// check if 2 reports are different enough
bool diff_report(sony_ds4_report_t const* rpt1, sony_ds4_report_t const* rpt2)
{
  bool result;

  // x, y, z, rz must different than 2 to be counted
  result = diff_than_2(rpt1->x, rpt2->x) || diff_than_2(rpt1->y , rpt2->y ) ||
           diff_than_2(rpt1->z, rpt2->z) || diff_than_2(rpt1->rz, rpt2->rz);

  // check the reset with mem compare
  result |= memcmp(&rpt1->rz + 1, &rpt2->rz + 1, sizeof(sony_ds4_report_t)-4);

  return result;
}

void process_sony_ds4(uint8_t const* report, uint16_t len)
{
  const char* dpad_str[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW", "none" };

  // previous report used to compare for changes
  static sony_ds4_report_t prev_report = { 0 };

  uint8_t const report_id = report[0];
  report++;
  len--;

  // all buttons state is stored in ID 1
  if (report_id == 1)
  {
    sony_ds4_report_t ds4_report;
    memcpy(&ds4_report, report, sizeof(ds4_report));

    // counter is +1, assign to make it easier to compare 2 report
    prev_report.counter = ds4_report.counter;

    // only print if changes since it is polled ~ 5ms
    // Since count+1 after each report and  x, y, z, rz fluctuate within 1 or 2
    // We need more than memcmp to check if report is different enough
    if ( diff_report(&prev_report, &ds4_report) )
    {
      //printf("(x, y, z, rz) = (%u, %u, %u, %u)\r\n", ds4_report.x, ds4_report.y, ds4_report.z, ds4_report.rz);
      //printf("DPad = %s ", dpad_str[ds4_report.dpad]);

      if (ds4_report.square   ) (PORT->Group[0].OUTSET.reg = (1 << 16));
      if (ds4_report.cross    ) (PORT->Group[0].OUTCLR.reg = (1 << 16));
      if (ds4_report.circle   ) (PORT->Group[0].OUTSET.reg = (1 << 17));
      if (ds4_report.triangle ) (PORT->Group[0].OUTCLR.reg = (1 << 17));

      if (ds4_report.l1       ) ;
      if (ds4_report.r1       ) (PORT->Group[0].OUTTGL.reg = (1 << 17));
      if (ds4_report.l2       ) ;
      if (ds4_report.r2       ) ;

      if (ds4_report.share    ) ;
      if (ds4_report.option   ) ;
      if (ds4_report.l3       ) (PORT->Group[0].OUTTGL.reg = (1 << 16));
      if (ds4_report.r3       ) ;

      if (ds4_report.ps       ) ;
      if (ds4_report.tpad     ) ;

      //printf("\r\n");
    }

    prev_report = ds4_report;
  }
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  if ( is_sony_ds4(dev_addr) )
  {
    process_sony_ds4(report, len);
  }

  // continue to request to receive report
  if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
    //printf("Error: cannot request to receive report\r\n");
  }
}
