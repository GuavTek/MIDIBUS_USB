/*
 * I2S_driver.h
 *
 * Created: 01/07/2022 14:04:57
 *  Author: GuavTek
 */ 


#ifndef I2S_DRIVER_H_
#define I2S_DRIVER_H_

#include "sam.h"
#include "asf.h"

enum {
	I2S_FS_LEFT_MAYBE,
	I2S_FS_RIGHT_MAYBE
} i2s_fs_state;

void i2s_init(uint32_t samplerate);
void i2s_set_freq(uint32_t samplerate);
void i2s_set_output_wordsize(uint8_t size);
void i2s_set_input_wordsize(uint8_t size);

#endif /* I2S_DRIVER_H_ */