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
#include "DMA_driver.h"
#include "I2S_driver.h"
#include "MIDI_Driver.h"
#include "tusb.h"


MCP2517_C CAN(SERCOM5);

void CAN_Receive(CAN_Rx_msg_t* data);
void USB_Init();
void audio_task(void);
void midi_task(void);
void audio_dma_init();

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

// DMA descriptors
DMA_Descriptor_t base_descriptor[2] __attribute__ ((aligned(16)));
DMA_Descriptor_t wrback_descriptor[2] __attribute__ ((aligned(16)));
DMA_Descriptor_t transact_descriptor[4] __attribute__ ((aligned(16)));

DMA_Descriptor_t* i2s_rx_descriptor_a = &base_descriptor[0];
DMA_Descriptor_t* i2s_rx_descriptor_b = &transact_descriptor[0];
DMA_Descriptor_t* i2s_rx_descriptor_wb = &wrback_descriptor[0];

DMA_Descriptor_t* i2s_tx_descriptor_a = &base_descriptor[1];
DMA_Descriptor_t* i2s_tx_descriptor_b = &transact_descriptor[1];
DMA_Descriptor_t* i2s_tx_descriptor_wb = &wrback_descriptor[1];


// For debugging
volatile Dmac* tempDMA;

int main(void)
{
	system_init();
	CAN.Init(CAN_CONF, SPI_CONF);
	CAN.Set_Rx_Callback(CAN_Receive);
	i2s_init(44100);
	
	dma_init(base_descriptor, wrback_descriptor);
	audio_dma_init();
	
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
	
	
			tempDMA = DMAC;
    while (1) 
    {
		CAN.State_Machine();
		
		// USB tasks
		tud_task();
		//tuh_task();
		
		audio_task();
		midi_task();
		
		static uint32_t timrr = 0;
		if (timrr < system_ticks/**/)	{
			timrr = system_ticks + blinkTime;
			PORT->Group[0].OUTTGL.reg = 1 << 17;
			
			
			// Bruh, transfer error and underrun
			//if (I2S->INTFLAG.bit.RXRDY0){
			if (I2S->INTFLAG.bit.TXRDY1){
			//if (DMAC->CHINTFLAG.bit.TERR){
				PORT->Group[0].OUTSET.reg = 1 << 16;
				//I2S->INTFLAG.bit.TXUR1;
			} else {
				PORT->Group[0].OUTCLR.reg = 1 << 16;
			}
			
			
		}
		
		/*
		static uint32_t timrr2 = 0;
		if (timrr2 < system_ticks)	{
			timrr2 = system_ticks + blinkTime2;
			PORT->Group[0].OUTTGL.reg = 1 << 16;
		}
		/**/
		//if (USB->DEVICE.DeviceEndpoint[3].EPINTFLAG.reg){
		//	PORT->Group[0].OUTSET.reg = 1 << 16;
		//}
		
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

// Supported sample rates
const uint32_t sample_rates[] = {44100, 48000, 88200, 96000};
uint32_t current_sample_rate  = 44100;
#define N_SAMPLE_RATES  TU_ARRAY_SIZE(sample_rates)

enum
{
	VOLUME_CTRL_0_DB = 0,
	VOLUME_CTRL_10_DB = 2560,
	VOLUME_CTRL_20_DB = 5120,
	VOLUME_CTRL_30_DB = 7680,
	VOLUME_CTRL_40_DB = 10240,
	VOLUME_CTRL_50_DB = 12800,
	VOLUME_CTRL_60_DB = 15360,
	VOLUME_CTRL_70_DB = 17920,
	VOLUME_CTRL_80_DB = 20480,
	VOLUME_CTRL_90_DB = 23040,
	VOLUME_CTRL_100_DB = 25600,
	VOLUME_CTRL_SILENCE = 0x8000,
};

// Audio controls
// Current states
int8_t mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];       // +1 for master channel 0
int16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];    // +1 for master channel 0

// Buffer for microphone data
bool mic_active = false;
const uint16_t mic_buf_size = 64;
int32_t mic_buf[mic_buf_size];
uint32_t* const mic_buf_lo = (uint32_t*) &mic_buf[0];
uint32_t* const mic_buf_hi = (uint32_t*) &mic_buf[mic_buf_size/2];

// Buffer for speaker data
bool spk_active = false;
const uint16_t spk_buf_size = 256;
int32_t spk_buf[spk_buf_size];
uint32_t* const spk_buf_lo = (uint32_t*) &spk_buf[0];
uint32_t* const spk_buf_hi = (uint32_t*) &spk_buf[spk_buf_size/2];

// Speaker data size received in the last frame
int16_t spk_data_new;

// Resolution per format
const uint8_t resolutions_per_format[CFG_TUD_AUDIO_FUNC_1_N_FORMATS] = {CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_RX,
CFG_TUD_AUDIO_FUNC_1_FORMAT_2_RESOLUTION_RX};
// Current resolution, update on format change
uint8_t current_resolution_out;
uint8_t current_resolution_in;

const struct DMA_Channel_config dma_i2s_tx_conf = {
	.int_enable_complete = 0,
	.int_enable_suspend = 0,
	.int_enable_error = 0,
	.trigger_src = I2S_DMAC_ID_TX_1,
	.trigger_action = DMAC_CHCTRLB_TRIGACT_BEAT_Val,
	.arbitration_lvl = 2
};

const struct DMA_Channel_config dma_i2s_rx_conf = {
	.int_enable_complete = 0,
	.int_enable_suspend = 0,
	.int_enable_error = 0,
	.trigger_src = I2S_DMAC_ID_RX_0,
	.trigger_action = DMAC_CHCTRLB_TRIGACT_BEAT_Val,
	.arbitration_lvl = 2
};



// Helper for clock get requests
static bool tud_audio_clock_get_request(uint8_t rhport, audio_control_request_t const *request)
{
	TU_ASSERT(request->bEntityID == UAC2_ENTITY_CLOCK);

	if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ)
	{
		if (request->bRequest == AUDIO_CS_REQ_CUR)
		{
			TU_LOG1("Clock get current freq %lu\r\n", current_sample_rate);

			audio_control_cur_4_t curf = { tu_htole32(current_sample_rate) };
			return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &curf, sizeof(curf));
		}
		else if (request->bRequest == AUDIO_CS_REQ_RANGE)
		{
			audio_control_range_4_n_t(N_SAMPLE_RATES) rangef =
			{
				.wNumSubRanges = tu_htole16(N_SAMPLE_RATES)
			};
			TU_LOG1("Clock get %d freq ranges\r\n", N_SAMPLE_RATES);
			for(uint8_t i = 0; i < N_SAMPLE_RATES; i++)
			{
				rangef.subrange[i].bMin = sample_rates[i];
				rangef.subrange[i].bMax = sample_rates[i];
				rangef.subrange[i].bRes = 0;
				TU_LOG1("Range %d (%d, %d, %d)\r\n", i, (int)rangef.subrange[i].bMin, (int)rangef.subrange[i].bMax, (int)rangef.subrange[i].bRes);
			}
			
			return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &rangef, sizeof(rangef));
		}
	}
	else if (request->bControlSelector == AUDIO_CS_CTRL_CLK_VALID &&
	request->bRequest == AUDIO_CS_REQ_CUR)
	{
		audio_control_cur_1_t cur_valid = { .bCur = 1 };
		TU_LOG1("Clock get is valid %u\r\n", cur_valid.bCur);
		return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &cur_valid, sizeof(cur_valid));
	}
	TU_LOG1("Clock get request not supported, entity = %u, selector = %u, request = %u\r\n",
	request->bEntityID, request->bControlSelector, request->bRequest);
	return false;
}

// Helper for clock set requests
static bool tud_audio_clock_set_request(uint8_t rhport, audio_control_request_t const *request, uint8_t const *buf)
{
	(void)rhport;

	TU_ASSERT(request->bEntityID == UAC2_ENTITY_CLOCK);
	TU_VERIFY(request->bRequest == AUDIO_CS_REQ_CUR);

	if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ)
	{
		TU_VERIFY(request->wLength == sizeof(audio_control_cur_4_t));

		current_sample_rate = ((audio_control_cur_4_t const *)buf)->bCur;
		
		i2s_set_freq(current_sample_rate);		
		
		TU_LOG1("Clock set current freq: %ld\r\n", current_sample_rate);

		return true;
	}
	else
	{
		TU_LOG1("Clock set request not supported, entity = %u, selector = %u, request = %u\r\n",
		request->bEntityID, request->bControlSelector, request->bRequest);
		return false;
	}
}

// Helper for feature unit get requests
static bool tud_audio_feature_unit_get_request(uint8_t rhport, audio_control_request_t const *request)
{
	TU_ASSERT(request->bEntityID == UAC2_ENTITY_SPK_FEATURE_UNIT);

	if (request->bControlSelector == AUDIO_FU_CTRL_MUTE && request->bRequest == AUDIO_CS_REQ_CUR)
	{
		audio_control_cur_1_t mute1 = { .bCur = mute[request->bChannelNumber] };
		TU_LOG1("Get channel %u mute %d\r\n", request->bChannelNumber, mute1.bCur);
		return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &mute1, sizeof(mute1));
	}
	else if (UAC2_ENTITY_SPK_FEATURE_UNIT && request->bControlSelector == AUDIO_FU_CTRL_VOLUME)
	{
		if (request->bRequest == AUDIO_CS_REQ_RANGE)
		{
			audio_control_range_2_n_t(1) range_vol = {
				tu_htole16(1),		// wNumSubRanges
				{ tu_htole16(-VOLUME_CTRL_50_DB), tu_htole16(VOLUME_CTRL_0_DB), tu_htole16(256) }		// subrange
			};
			TU_LOG1("Get channel %u volume range (%d, %d, %u) dB\r\n", request->bChannelNumber,
			range_vol.subrange[0].bMin / 256, range_vol.subrange[0].bMax / 256, range_vol.subrange[0].bRes / 256);
			return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &range_vol, sizeof(range_vol));
		}
		else if (request->bRequest == AUDIO_CS_REQ_CUR)
		{
			audio_control_cur_2_t cur_vol = { .bCur = tu_htole16(volume[request->bChannelNumber]) };
			TU_LOG1("Get channel %u volume %d dB\r\n", request->bChannelNumber, cur_vol.bCur / 256);
			return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &cur_vol, sizeof(cur_vol));
		}
	}
	TU_LOG1("Feature unit get request not supported, entity = %u, selector = %u, request = %u\r\n",
	request->bEntityID, request->bControlSelector, request->bRequest);

	return false;
}

// Helper for feature unit set requests
static bool tud_audio_feature_unit_set_request(uint8_t rhport, audio_control_request_t const *request, uint8_t const *buf)
{
	(void)rhport;

	TU_ASSERT(request->bEntityID == UAC2_ENTITY_SPK_FEATURE_UNIT);
	TU_VERIFY(request->bRequest == AUDIO_CS_REQ_CUR);

	if (request->bControlSelector == AUDIO_FU_CTRL_MUTE)
	{
		TU_VERIFY(request->wLength == sizeof(audio_control_cur_1_t));

		mute[request->bChannelNumber] = ((audio_control_cur_1_t const *)buf)->bCur;

		TU_LOG1("Set channel %d Mute: %d\r\n", request->bChannelNumber, mute[request->bChannelNumber]);

		return true;
	}
	else if (request->bControlSelector == AUDIO_FU_CTRL_VOLUME)
	{
		TU_VERIFY(request->wLength == sizeof(audio_control_cur_2_t));

		volume[request->bChannelNumber] = ((audio_control_cur_2_t const *)buf)->bCur;

		TU_LOG1("Set channel %d volume: %d dB\r\n", request->bChannelNumber, volume[request->bChannelNumber] / 256);

		return true;
	}
	else
	{
		TU_LOG1("Feature unit set request not supported, entity = %u, selector = %u, request = %u\r\n",
		request->bEntityID, request->bControlSelector, request->bRequest);
		return false;
	}
}

//--------------------------------------------------------------------+
// Application Callback API Implementations
//--------------------------------------------------------------------+

// Invoked when audio class specific get request received for an entity
bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
	audio_control_request_t const *request = (audio_control_request_t const *)p_request;

	if (request->bEntityID == UAC2_ENTITY_CLOCK)
	return tud_audio_clock_get_request(rhport, request);
	if (request->bEntityID == UAC2_ENTITY_SPK_FEATURE_UNIT)
	return tud_audio_feature_unit_get_request(rhport, request);
	else
	{
		TU_LOG1("Get request not handled, entity = %d, selector = %d, request = %d\r\n",
		request->bEntityID, request->bControlSelector, request->bRequest);
	}
	return false;
}

// Invoked when audio class specific set request received for an entity
bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *buf)
{
	audio_control_request_t const *request = (audio_control_request_t const *)p_request;

	if (request->bEntityID == UAC2_ENTITY_SPK_FEATURE_UNIT)
	return tud_audio_feature_unit_set_request(rhport, request, buf);
	if (request->bEntityID == UAC2_ENTITY_CLOCK)
	return tud_audio_clock_set_request(rhport, request, buf);
	TU_LOG1("Set request not handled, entity = %d, selector = %d, request = %d\r\n",
	request->bEntityID, request->bControlSelector, request->bRequest);

	return false;
}

bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
	(void)rhport;

	uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
	uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));

	if (ITF_NUM_AUDIO_STREAMING_SPK == itf && alt == 0) {
		//current_resolution_out = 0;
		blinkTime = 100;
		// Detach DMA
		dma_suspend(1);
		spk_active = false;
		// Clear DMA transactions?
	}
	
	if (ITF_NUM_AUDIO_STREAMING_MIC == itf && alt == 0){
		//current_resolution_in = 0;
		blinkTime2 = 100;
		// Detach DMA
		dma_suspend(0);
		mic_active = false;
		// Clear DMA transactions?
	}
	
	return true;
}

bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
	(void)rhport;
	uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
	uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));
		
	TU_LOG2("Set interface %d alt %d\r\n", itf, alt);
	if (ITF_NUM_AUDIO_STREAMING_SPK == itf && alt != 0) {
		current_resolution_out = resolutions_per_format[alt-1];
		blinkTime = 40;
		i2s_set_output_wordsize(current_resolution_out);
		// Attach DMA
		//dma_resume(1);
		
		// Clear buffer when streaming format is changed
		spk_data_new = 0;
		i2s_tx_descriptor_a->btctrl.valid = 0;
		i2s_tx_descriptor_b->btctrl.valid = 0;
		//i2s_tx_descriptor_wb->btctrl.valid = 0;
		spk_active = true;
	}
	
	if (ITF_NUM_AUDIO_STREAMING_MIC == itf && alt != 0) {
		current_resolution_in = resolutions_per_format[alt-1];
		blinkTime2 = 40;
		i2s_set_input_wordsize(current_resolution_in);
		// Attach DMA
		//dma_resume(0);
		
		i2s_rx_descriptor_a->btctrl.valid = 0;
		i2s_rx_descriptor_b->btctrl.valid = 0;
		//i2s_rx_descriptor_wb->btctrl.valid = 0;
		mic_active = true;
	}

	return true;
}

bool tud_audio_rx_done_pre_read_cb(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting)
{
	(void)rhport;
	(void)func_id;
	(void)ep_out;
	(void)cur_alt_setting;
	
	spk_data_new += n_bytes_received;
	
	return true;
}

bool tud_audio_rx_done_post_read_cb(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting)
{
	(void)rhport;
	(void)func_id;
	(void)ep_out;
	(void)cur_alt_setting;
	
	//if (spk_data_new > 0){
	//	PORT->Group[0].OUTSET.reg = 1 << 16;
	//}
	
	//spk_data_new += n_bytes_received;
	
	return true;
}

bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
	(void)rhport;
	(void)itf;
	(void)ep_in;
	(void)cur_alt_setting;

	// This callback could be used to fill microphone data separately
	return true;
}

//--------------------------------------------------------------------+
// AUDIO Task
//--------------------------------------------------------------------+

uint32_t* const i2s_tx_reg = (uint32_t*) &I2S->DATA[1].reg;
uint32_t* const i2s_rx_reg = (uint32_t*) &I2S->DATA[0].reg;

void audio_dma_init(){
	DMA_BTCTRL_t tempCtrl; /*{
		.valid = 0,
		.evosel = DMAC_BTCTRL_EVOSEL_BEAT_Val,
		.blockact = DMAC_BTCTRL_BLOCKACT_NOACT_Val,
		.reserved = 0,
		.beatsize = DMAC_BTCTRL_BEATSIZE_HWORD_Val,
		.srcinc = 0,
		.dstinc = 1,
		.stepsel = 0,
		.stepsize = 0
	}; */
	tempCtrl.word = 0;
	tempCtrl.evosel = DMAC_BTCTRL_EVOSEL_BEAT_Val;
	tempCtrl.blockact = DMAC_BTCTRL_BLOCKACT_NOACT_Val;
	tempCtrl.beatsize = DMAC_BTCTRL_BEATSIZE_HWORD_Val;
	tempCtrl.stepsel = DMAC_BTCTRL_STEPSEL_SRC_Val;
	tempCtrl.dstinc = 1;
	tempCtrl.srcinc = 0;
	
	// RX
	dma_set_descriptor(i2s_rx_descriptor_a, mic_buf_size, i2s_rx_reg, mic_buf_lo, i2s_rx_descriptor_b, tempCtrl);
	dma_set_descriptor(i2s_rx_descriptor_b, mic_buf_size, i2s_rx_reg, mic_buf_hi, i2s_rx_descriptor_a, tempCtrl);
	
	//tempCtrl.valid = 1;
	//dma_set_descriptor(&base_descriptor[0], mic_buf_size, i2s_rx_reg, mic_buf_lo, &transact_descriptor[0], tempCtrl);
	
	
	tempCtrl.stepsel = DMAC_BTCTRL_STEPSEL_DST_Val;
	tempCtrl.valid = 0;
	tempCtrl.srcinc = 1;
	tempCtrl.dstinc = 0;
	
	// TX
	dma_set_descriptor(i2s_tx_descriptor_a, spk_buf_size, spk_buf_lo, i2s_tx_reg, i2s_tx_descriptor_b, tempCtrl);
	dma_set_descriptor(i2s_tx_descriptor_b, spk_buf_size, spk_buf_hi, i2s_tx_reg, i2s_tx_descriptor_a, tempCtrl);
	
	//tempCtrl.valid = 1;
	//dma_set_descriptor(&base_descriptor[1], spk_buf_size, spk_buf_lo, i2s_tx_reg, &transact_descriptor[2], tempCtrl);
	
	dma_attach(0, dma_i2s_rx_conf);
	dma_suspend(0);
	dma_attach(1, dma_i2s_tx_conf);
	dma_suspend(1);
}

volatile uint64_t dropped_bytes = 0;

void audio_task(void)
{
	
	// Is DMAC processing descriptor?
	if (i2s_rx_descriptor_wb->next_descriptor == i2s_rx_descriptor_b){
		i2s_rx_descriptor_a->btctrl.valid = 0;
	} else if (i2s_rx_descriptor_wb->next_descriptor == i2s_rx_descriptor_a){
		i2s_rx_descriptor_b->btctrl.valid = 0;
	}
	
	if (i2s_tx_descriptor_wb->next_descriptor == i2s_tx_descriptor_b){
		i2s_tx_descriptor_a->btctrl.valid = 0;
	} else if (i2s_tx_descriptor_wb->next_descriptor == i2s_tx_descriptor_a){
		i2s_tx_descriptor_b->btctrl.valid = 0;
	}
	/**/
	
	// Speaker data waiting?
	if (spk_data_new > 0){
		// Check if buffer is valid, and is not being processed
		if ( (!i2s_tx_descriptor_a->btctrl.valid) && (i2s_tx_descriptor_wb->next_descriptor != i2s_tx_descriptor_b) )	{
			uint8_t data_shift = (current_resolution_out + 8) >> 4;
			DMA_BTCTRL_t tempCtrl;
			tempCtrl.word = 0;
			tempCtrl.evosel = DMAC_BTCTRL_EVOSEL_BEAT_Val;
			tempCtrl.blockact = DMAC_BTCTRL_BLOCKACT_NOACT_Val;
			tempCtrl.stepsel = DMAC_BTCTRL_STEPSEL_DST_Val;
			tempCtrl.beatsize = data_shift;
			tempCtrl.srcinc = 1;
			tempCtrl.valid = 1;
	
			uint16_t new_data_size;
			new_data_size = tud_audio_read(spk_buf_lo, spk_buf_size * 2 );
			spk_data_new -= new_data_size;
			new_data_size >>= data_shift;
			
			if (new_data_size == 0){
				dropped_bytes += spk_data_new;
				spk_data_new = 0;
			} else {
				dma_set_descriptor(i2s_tx_descriptor_a, new_data_size, spk_buf_lo, i2s_tx_reg, i2s_tx_descriptor_b, tempCtrl);
				//dma_set_descriptor(i2s_tx_descriptor_a, new_data_size, data_shift);
				dma_resume(1);
			}
		} else if ( (!i2s_tx_descriptor_b->btctrl.valid) && (i2s_tx_descriptor_wb->next_descriptor != i2s_tx_descriptor_a) ){
			uint8_t data_shift = (current_resolution_out + 8) >> 4;
			DMA_BTCTRL_t tempCtrl;
			tempCtrl.word = 0;
			tempCtrl.evosel = DMAC_BTCTRL_EVOSEL_BEAT_Val;
			tempCtrl.blockact = DMAC_BTCTRL_BLOCKACT_NOACT_Val;
			tempCtrl.stepsel = DMAC_BTCTRL_STEPSEL_DST_Val;
			tempCtrl.beatsize = data_shift;
			tempCtrl.srcinc = 1;
			tempCtrl.valid = 1;
			
			uint16_t new_data_size;
			new_data_size = tud_audio_read(spk_buf_hi, spk_buf_size * 2 );
			spk_data_new -= new_data_size;
			new_data_size >>= data_shift;
			
			if (new_data_size == 0){
				dropped_bytes += spk_data_new;
				spk_data_new = 0;
			} else {
				dma_set_descriptor(i2s_tx_descriptor_b, new_data_size, spk_buf_hi, i2s_tx_reg, i2s_tx_descriptor_a, tempCtrl);
				//dma_set_descriptor(i2s_tx_descriptor_b, new_data_size, data_shift);
				dma_resume(1);
			}
			
			
		}
		
		/*
		// Is transaction active?
		if ( !DMAC->INTSTATUS.bit.CHINT1 ) {
			// Resume channel
			dma_resume(1);
		}*/
	}
	
	if (mic_active){
		if ( (!i2s_rx_descriptor_a->btctrl.valid) && (i2s_rx_descriptor_wb->next_descriptor != i2s_rx_descriptor_b) )	{
			uint8_t data_shift = (current_resolution_in + 8) >> 4;
			DMA_BTCTRL_t tempCtrl;
			tempCtrl.word = 0;
			tempCtrl.evosel = DMAC_BTCTRL_EVOSEL_BEAT_Val;
			tempCtrl.blockact = DMAC_BTCTRL_BLOCKACT_NOACT_Val;
			tempCtrl.beatsize = data_shift;
			tempCtrl.stepsel = DMAC_BTCTRL_STEPSEL_SRC_Val;
			tempCtrl.dstinc = 1;
			tempCtrl.valid = 1;
			
			tud_audio_write(mic_buf_lo, mic_buf_size * 2 );
			
			
			//dma_set_descriptor(i2s_rx_descriptor_a, ((mic_buf_size*2) >> data_shift), i2s_rx_reg, mic_buf_lo, i2s_rx_descriptor_b, tempCtrl);
			dma_set_descriptor(i2s_rx_descriptor_a, ((mic_buf_size*2) >> data_shift), data_shift);
		} else if ( (!i2s_rx_descriptor_b->btctrl.valid) && (i2s_rx_descriptor_wb->next_descriptor != i2s_rx_descriptor_a) ){
			uint8_t data_shift = (current_resolution_in + 8) >> 4;
			DMA_BTCTRL_t tempCtrl;
			tempCtrl.word = 0;
			tempCtrl.evosel = DMAC_BTCTRL_EVOSEL_BEAT_Val;
			tempCtrl.blockact = DMAC_BTCTRL_BLOCKACT_NOACT_Val;
			tempCtrl.beatsize = data_shift;
			tempCtrl.stepsel = DMAC_BTCTRL_STEPSEL_SRC_Val;
			tempCtrl.dstinc = 1;
			tempCtrl.valid = 1;
			
			tud_audio_write(mic_buf_hi, mic_buf_size * 2 );
			
			//dma_set_descriptor(i2s_rx_descriptor_b, ((mic_buf_size*2) >> data_shift), i2s_rx_reg, mic_buf_hi, i2s_rx_descriptor_a, tempCtrl);
			dma_set_descriptor(i2s_rx_descriptor_b, ((mic_buf_size*2) >> data_shift), data_shift);
		}
		
		// Has pending interrupts?
		//if ( !DMAC->INTSTATUS.bit.CHINT0 ) {
			// Resume channel
			dma_resume(0);
		//}
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