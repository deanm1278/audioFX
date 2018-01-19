/*
 * ak4558.cpp
 *
 *  Created on: Jan 19, 2018
 *      Author: deanm
 */

#include "ak4558.h"

bool ak4558::begin(uint8_t addr) {
	_addr = addr;
	_i2c->begin();

	//power up DAC and ADC channels
	_powerManagement.reg = 0x1F;
	write8(AK4558_POWER_CONTROL, _powerManagement.reg);

	//power up PLL
	_PLLControl.bit.PMPLL = 1;
	_PLLControl.bit.PLL = 0x02; //BICK refreence 64fs
	write8(AK4558_PLL, _PLLControl.reg);

	//set format
	_control1.bit.DIF = 0x03; //24 bit i2s for now
	write8(AK4558_CONTROL_1, _control1.reg);

	_control2.bit.MCKS = 0;
	write8(AK4558_CONTROL_2, _control2.reg);

	_modeControl.bit.FS = 0x08; //48kHz
	write8(AK4558_MODE_CONTROL, _modeControl.reg);

	return true;
}

void ak4558::init() {
	//set register defaults
	_powerManagement.reg = 0x01;
	_PLLControl.reg = 0x04;
	_DACTDM.reg = 0x00;
	_control1.reg = 0x38;
	_control2.reg = 0x10;
	_modeControl.reg = 0x2A;
	_filterSetting.reg = 0x29;
	_HPF.reg = 0x07;
	_LOUTVolume.reg = 0xFF;
	_ROUTVolume.reg = 0xFF;
}

void ak4558::write8(uint8_t reg, uint8_t val) {
	_i2c->beginTransmission(_addr);
	_i2c->write(reg);
	_i2c->write(val);
	_i2c->endTransmission();
}

uint8_t ak4558::read8(uint8_t reg) {
	_i2c->beginTransmission(_addr);
	_i2c->write(reg);
	_i2c->endTransmission();

	_i2c->requestFrom(_addr, 1);
	return _i2c->read();
}
