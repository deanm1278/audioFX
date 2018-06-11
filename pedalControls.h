/*
 * pedalControls.h
 *
 *  Created on: Jun 10, 2018
 *      Author: Dean
 */

#ifndef AUDIOFX_PEDALCONTROLS_H_
#define AUDIOFX_PEDALCONTROLS_H_

#include <Arduino.h>
#include <SPI.h>

#define PEDAL_NUM_ADC 6

class PedalControls {
public:
	PedalControls(SPIClass *spi, uint8_t ss, uint8_t start) : _spi(spi), _ss(ss), _pin(start) {}
	~PedalControls() {}

	bool begin();

	void pause() { digitalWrite(_pin, HIGH); }
	void resume() { digitalWrite(_pin, LOW); }

	struct pedalState {
		uint16_t adcPrimary[PEDAL_NUM_ADC];
		uint16_t adcAlt[PEDAL_NUM_ADC];
		union {
			struct {
				uint8_t footswitch1:1;
				uint8_t footswitch2:1;
				uint8_t alt:1;
			} bit;
			uint8_t reg;
		} btns;
	};
	struct pedalState state;
private:
	SPIClass *_spi;
	uint8_t _ss, _pin;
};

extern PedalControls controls;

#endif /* AUDIOFX_PEDALCONTROLS_H_ */
