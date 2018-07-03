/*
 * adau17x1.cpp
 *
 *  Created on: Apr 22, 2018
 *      Author: Dean
 */


#include "adau17x1.h"
#include <Wire.h>
#include <Arduino.h>

#define LOAD_FROM_SIGMA_STUDIO

#ifdef LOAD_FROM_SIGMA_STUDIO
#include "SigmaStudio/export_IC_1.h"
static uint8_t _global_addr;
#endif

bool adau17x1::begin(uint8_t addr)
{
	/*  1. Apply power to the ADAU1761.
	 *	2. Lock the PLL to the input clock (if using the PLL).
	 *	3. Enable the core clock.
	 *	4. Load the register setting
	 */
	_i2c->begin();
	_addr = addr;

#ifndef LOAD_FROM_SIGMA_STUDIO
	//fs = 48khz from a 12.288 Mhz MCLK input
	for(int i=0; i<6; i++) _PLLControl.reg[i] = 0;
	writeReg(ADAU17X1_PLL_CONTROL, _PLLControl.reg, 6);

	_PLLControl.bit.R = 4; //integer
	_PLLControl.bit.PLLEN = 1;
	writeReg(ADAU17X1_PLL_CONTROL, _PLLControl.reg, 6);

	//wait for lock
	bool locked = false;
	while(!locked){
		readReg(ADAU17X1_PLL_CONTROL, _PLLControl.reg, 6);
		delay(1);
		locked = _PLLControl.bit.Lock;
	}

	_clockControl.reg = 0;
	_clockControl.bit.CLKSRC = 1; //PLL input
	_clockControl.bit.COREN = 1; //enable the core clock
	writeReg(ADAU17X1_CLOCK_CONTROL, &_clockControl.reg);
#else
	//load sigmastudio generated firmware
	_global_addr = addr;
	default_download_IC_1();
#endif

	//select ADC as input
	readReg(ADAU17X1_MICBIAS, &_recordMicBias.reg);
	readReg(ADAU17X1_DIG_DETECT, &_digDetect.reg);
	readReg(ADAU17X1_ADC_CONTROL, &_ADCControl.reg);

	_digDetect.bit.JDFUNC = 0;
	_recordMicBias.bit.MBIEN = 0;
	_ADCControl.bit.INSEL = 0;

	writeReg(ADAU17X1_MICBIAS, &_recordMicBias.reg);
	writeReg(ADAU17X1_DIG_DETECT, &_digDetect.reg);
	writeReg(ADAU17X1_ADC_CONTROL, &_ADCControl.reg);

	//set volume
	_playHPLeftVol.bit.LHPVOL = 0x39; //0dB
	_playHPLeftVol.bit.HPEN = 1;
	_playHPLeftVol.bit.LHPM = 1;
	_playHPRightVol.bit.RHPVOL = 0x39;
	_playHPRightVol.bit.HPMODE = 1;
	_playHPRightVol.bit.RHPM = 1;
	writeReg(ADAU17X1_PLAY_HP_LEFT_VOL, &_playHPLeftVol.reg);
	writeReg(ADAU17X1_PLAY_HP_RIGHT_VOL, &_playHPRightVol.reg);

	//set sample rate
	readReg(ADAU17X1_CONVERTER0, &_converter0.reg);
	readReg(ADAU17X1_SERIAL_SAMPLING_RATE, &_serialPortSamplingRate.reg);
	readReg(ADAU17X1_DSP_SAMPLING_RATE, &_DSPSamplingRateSetting.reg);

	_converter0.bit.CONVSR = 0; //48khz
	_serialPortSamplingRate.bit.SPSR = 0; //48khz
	_DSPSamplingRateSetting.bit.DSPSR = 1; //48khz

	writeReg(ADAU17X1_CONVERTER0, &_converter0.reg);
	writeReg(ADAU17X1_SERIAL_SAMPLING_RATE, &_serialPortSamplingRate.reg);
	writeReg(ADAU17X1_DSP_SAMPLING_RATE, &_DSPSamplingRateSetting.reg);

	//enable mixer
	_recMixerLeft0.reg = 0x0B;
	_recMixerRight0.reg = 0x0B;
	writeReg(ADAU17X1_REC_MIXER_LEFT_0, &_recMixerLeft0.reg);
	writeReg(ADAU17X1_REC_MIXER_RIGHT_0, &_recMixerRight0.reg);

	return true;
}

void adau17x1::writeReg(uint16_t reg, uint8_t *data, uint8_t count)
{
	_i2c->beginTransmission(_addr);
	_i2c->write((uint8_t)(reg >> 8));
	_i2c->write((uint8_t)reg & 0xFF);
	_i2c->write(data, count);
	_i2c->endTransmission();
}

void adau17x1::readReg(uint16_t reg, uint8_t *data, uint8_t count)
{
	_i2c->beginTransmission(_addr);
	_i2c->write((uint8_t)(reg >> 8));
	_i2c->write((uint8_t)reg & 0xFF);
	_i2c->endTransmission(false);
	for(int i_ = 0; i_ < 20000; i_++){ __asm__ volatile("NOP;"); }
	_i2c->requestFrom(_addr, count);
	while(count--)
		*data++ = _i2c->read();
}

#ifdef LOAD_FROM_SIGMA_STUDIO

void SIGMA_WRITE_REGISTER_BLOCK(
		uint16_t  devAddress,
		uint16_t  regAddr,
		uint16_t  length,
		uint8_t   *pRegData)
{
	Wire.beginTransmission(_global_addr);
	Wire.write((uint8_t)(regAddr >> 8));
	Wire.write((uint8_t)regAddr & 0xFF);
	Wire.write(pRegData, length);
	Wire.endTransmission();
}


void SIGMA_WRITE_DELAY(
		uint16_t  devAddress,
		uint16_t  length,
		uint8_t   *pData)
{
	uint16_t i;
	uint32_t d;

	for (i=0u; i<length; i++)
	{
		for (d=0u; d < (40000u * (uint32_t)pData[i]); d++)
		{
			asm("nop;");
		}
	}
}

#endif


