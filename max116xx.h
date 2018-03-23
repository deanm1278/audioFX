/*
 * max116xx.h
 *
 *  Created on: Mar 23, 2018
 *      Author: Dean
 */

#ifndef AUDIOFX_MAX116XX_H_
#define AUDIOFX_MAX116XX_H_

#include <Arduino.h>
#include "SPI.h"

#define MAX11618_NUM_CHANNELS 4
#define MAX11619_NUM_CHANNELS 4
#define MAX11620_NUM_CHANNELS 8
#define MAX11621_NUM_CHANNELS 8
#define MAX11624_NUM_CHANNELS 16
#define MAX11625_NUM_CHANNELS 16

enum {
	MAX116XX_MODE_SCAN_UP_TO = 0,
	MAX116XX_MODE_SCAN_N_TO_HIGHEST,
	MAX116XX_MODE_SCAN_N_REPEATEDLY,
	MAX116XX_MODE_NO_SCAN,
};

class max116xx {
public:
	max116xx(int csPin, SPIClass *spi=&SPI) : _cs(csPin), _spi(spi) {
		//set reserved bits
		_conversion.bit.reserved = 1;
		_setup.bit.reserved = 1;
		_averaging.bit.reserved = 1;
		_reset.bit.reserved = 1;
	}
	~max116xx() {}

	bool begin();

	void startConversion(uint8_t channel, uint8_t mode=MAX116XX_MODE_SCAN_UP_TO);

	void readFifo(uint16_t *data, uint8_t num);
private:
	int _cs;
	SPIClass *_spi;

	void writeReg(uint8_t reg);

/*=========================================================================
REGISTER BITFIELDS
-----------------------------------------------------------------------*/
	union conversion {
		struct {
			uint8_t :1;
			uint8_t SCAN:2;
			uint8_t CHSEL:4;
			uint8_t reserved:1;
		} bit;
		uint8_t reg;
	};
	conversion _conversion;

	union setup {
		struct {
			uint8_t :2;
			uint8_t REFSEL:2;
			uint8_t CKSEL:2;
			uint8_t reserved:2;
		} bit;
		uint8_t reg;
	};
	setup _setup;

	union averaging {
		struct{
			uint8_t NSCAN:2;
			uint8_t NAVG:2;
			uint8_t AVGON:1;
			uint8_t reserved:3;
		} bit;
		uint8_t reg;
	};
	averaging _averaging;

	union reset {
		struct{
			uint8_t :3;
			uint8_t NRESET:1;
			uint8_t reserved;
		} bit;
		uint8_t reg;
	};
	reset _reset;
};


#endif /* AUDIOFX_MAX116XX_H_ */
