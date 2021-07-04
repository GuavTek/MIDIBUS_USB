/*
 * MIDIBUS_USB.cpp
 *
 * Created: 26/06/2021 19:48:09
 * Author : GuavTek
 */ 



#include "sam.h"
#include "asf.h"
#include "MIDI_Driver.h"
#include "SPI.h"

const spi_config_t SPI_CONF = {
	.sercomNum = 5,
	.dipoVal = 0x3,
	.dopoVal = 0x0,
	.speed = 1000000,
	.pin_cs = PIN_PB22,
	.pinmux_mosi = PINMUX_PB02D_SERCOM5_PAD0,
	.pinmux_miso = PINMUX_PB23D_SERCOM5_PAD3,
	.pinmux_sck = PINMUX_PB03D_SERCOM5_PAD1
};

SPI_C SPI_TEST(SERCOM5, SPI_CONF);

void SERCOM5_Handler(){
	SPI_TEST.Handler();
}

MIDI_C MIDI_USB(2);
MIDI_C MIDI_CAN(2);

int main(void)
{
	system_init();
    /* Replace with your application code */
    while (1) 
    {
		
    }
}
