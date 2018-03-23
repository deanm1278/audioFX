#include "max116xx.h"

static const SPISettings max116xxSettings(4000000, MSB_FIRST, SPI_MODE0);

bool max116xx::begin(void)
{
	pinMode(_cs, OUTPUT);

	_spi->begin();

	digitalWrite(_cs, HIGH);

	//reset everything
	_reset.bit.NRESET = 0;
	writeReg(_reset.reg);

	delay(10);

	//use external reference (3.3V)
	_setup.bit.REFSEL = 1;
	_setup.bit.CKSEL = 2; //clock mode 10
	writeReg(_setup.reg);

#if 0
	//average 8 conversions TODO: tune this
	_averaging.bit.AVGON = 1;
	_averaging.bit.NAVG = 1;
	writeReg(_averaging.reg);
#endif

	return true;
}

void max116xx::startConversion(uint8_t channel, uint8_t mode)
{
		_conversion.bit.SCAN = mode;
		_conversion.bit.CHSEL = channel;
		writeReg(_conversion.reg);
}

void max116xx::readFifo(uint16_t *data, uint8_t num)
{
	digitalWrite(_cs, LOW);
	_spi->beginTransaction(max116xxSettings);
	for(int i=0; i<num; i++){
		uint16_t val = _spi->transfer16(0x00000);
		*data++ = val >> 2;
	}
	_spi->endTransaction();
	digitalWrite(_cs, HIGH);
}

void max116xx::writeReg(uint8_t reg)
{
	digitalWrite(_cs, LOW);
	_spi->beginTransaction(max116xxSettings);

	_spi->transfer(reg);

	_spi->endTransaction();
	digitalWrite(_cs, HIGH);

}
