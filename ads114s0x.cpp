/*
 * ads114s0x.cpp
 *
 *  Created on: May 23, 2018
 *      Author: deanm
 */


#include "ads114s0x.h"

static SPISettings ads114s0x_settings(12000000, MSBFIRST, SPI_MODE1); //this can go as high as 12.5 Mhz (design dependent)

bool ads114s0x::begin(){
    pinMode(_cs, OUTPUT);
    if(_rdy > -1)
      pinMode(_rdy, INPUT);
      
    digitalWrite(_cs, HIGH);
    _spi->begin();
    uint8_t dev_id = readReg(ADS114S0X_ID) & 0x7;
    if(dev_id != ADS114S0x_DEV_ID_ADS114S08 && dev_id != ADS114S0x_DEV_ID_ADS114S06)
      return false;

    sendCommand(ADS114S0X_COMMAND_RESET);
    delay(100);
      
    //set some defaults
    _pga.reg = (7 << 5) | (1 << 3); //enable PGA, DELAY to 1 * tMOD
    writeReg(ADS114S0X_PGA, _pga.reg);

    _inpmux.reg = 0x0C; //aincom as negative input
    writeReg(ADS114S0X_INPMUX, _inpmux.reg);
    
    _datarate.reg = 0x3E; //low-latency filter, 4ksps, single shot
    writeReg(ADS114S0X_DATARATE, _datarate.reg);

    //_vbias.reg = 0x40;
    //writeReg(ADS114S0X_VBIAS, _vbias.reg);

    _ref.reg = 0x00;
    //_ref.reg = 0x0A; //internal reference
    writeReg(ADS114S0X_REF, _ref.reg);

    _ofcal.reg = 0;
    writeReg(ADS114S0X_OFCAL0, _ofcal.reg);
    writeReg(ADS114S0X_OFCAL1, _ofcal.reg >> 8);

    return true;
}

int16_t ads114s0x::readChannel(int ch){
  if(ch > -1) setChannel(ch);
  sendCommand(ADS114S0X_COMMAND_START);
  return readData(false, true);
}

void ads114s0x::start(){
	sendCommand(ADS114S0X_COMMAND_START);
}

void ads114s0x::setChannel(uint8_t ch){
  _inpmux.bit.MUXP = ch;
  writeReg(ADS114S0X_INPMUX, _inpmux.reg);
}

void ads114s0x::setConversionMode(uint8_t mode){
	_datarate.bit.MODE = mode;
	writeReg(ADS114S0X_DATARATE, _datarate.reg);
}

void ads114s0x::writeReg(uint8_t reg, uint8_t value)
{ 
    digitalWrite(_cs, LOW);
    _spi->beginTransaction(ads114s0x_settings);
    _spi->transfer16(ADS114S0X_COMMAND_WREG | ((uint16_t)reg << 8));
    _spi->transfer(value);
    digitalWrite(_cs, HIGH);
    _spi->endTransaction();
}
uint8_t ads114s0x::readReg(uint8_t reg)
{
    byte b;
    digitalWrite(_cs, LOW);
    _spi->beginTransaction(ads114s0x_settings);
    _spi->transfer16(ADS114S0X_COMMAND_RREG | ((uint16_t)reg << 8));
    b = _spi->transfer(0x00);
    digitalWrite(_cs, HIGH);
    _spi->endTransaction();

    return b;
}

int ads114s0x::readData(bool byCommand, bool poll){
    int b;

    if(poll){
    	while(digitalRead(_rdy));
    }

    digitalWrite(_cs, LOW);
    _spi->beginTransaction(ads114s0x_settings);
    if(byCommand) _spi->transfer(ADS114S0X_COMMAND_RDATA);
    b = _spi->transfer16(0x00);
    digitalWrite(_cs, HIGH);
    _spi->endTransaction();

    return b;
}

void ads114s0x::sendCommand(uint8_t cmd)
{
    digitalWrite(_cs, LOW);
    _spi->beginTransaction(ads114s0x_settings);
    _spi->transfer(cmd);
    digitalWrite(_cs, HIGH);
    _spi->endTransaction();
}


