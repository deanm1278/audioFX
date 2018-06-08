/*
 * ak4558.h
 *
 *  Created on: Jan 19, 2018
 *      Author: deanm
 */

#ifndef AUDIOFX_AK4558_H_
#define AUDIOFX_AK4558_H_

#include "Wire.h"

#define AK4558_ADDR 0x10

//register definitions
enum {
	AK4558_POWER_CONTROL 	=	0x00,
	AK4558_PLL,
	AK4558_DAC_TDM,
	AK4558_CONTROL_1,
	AK4558_CONTROL_2,
	AK4558_MODE_CONTROL,
	AK4558_FILTER_SETTING,
	AK4558_HPF,
	AK4558_LOUT_VOLUME,
	AK4558_ROUT_VOLUME,
};

class ak4558 {
public:
	ak4558() : _i2c(&Wire) { }
	ak4558(TwoWire *wire) : _i2c(wire) { }
	~ak4558() {};

	bool begin(uint8_t addr = AK4558_ADDR);

private:
	TwoWire *_i2c;
	uint8_t _addr;

	void write8(uint8_t reg, uint8_t val);
	uint8_t read8(uint8_t reg);

/*=========================================================================
REGISTER BITFIELDS
-----------------------------------------------------------------------*/
	union powerManagement {
		struct {
			uint8_t RSTN:1;
			uint8_t PMDAL:1;
			uint8_t PMDAR:1;
			uint8_t PMADL:1;
			uint8_t PMADR:1;
		} bit;
		uint8_t reg;
	};
	powerManagement _powerManagement;

	union PLLControl {
		struct {
			uint8_t PMPLL:1;
			uint8_t PLL:4;
		} bit;
		uint8_t reg;
	};
	PLLControl _PLLControl;

	union DACTDM {
		struct {
			uint8_t SDS:2;
		} bit;
		uint8_t reg;
	};
	DACTDM _DACTDM;

	union control1 {
		struct {
			uint8_t SMUTE:1;
			uint8_t ATS:2;
			uint8_t DIF:3;
			uint8_t TDM:2;
		} bit;
		uint8_t reg;
	};
	control1 _control1;

	union control2 {
		struct {
			uint8_t ACKS:1;
			uint8_t DFS:2;
			uint8_t MCKS:2;
		} bit;
		uint8_t reg;
	};
	control2 _control2;

	union modeControl {
		struct {
			uint8_t LOPS:1;
			uint8_t BCKO:2;
			uint8_t FS:4;
		} bit;
		uint8_t reg;
	};
	modeControl _modeControl;

	union filterSetting {
		struct {
			uint8_t DEM:2;
			uint8_t SSLOW:1;
			uint8_t SDDA:1;
			uint8_t SLDA:1;
			uint8_t FIRDA:3;
		} bit;
		uint8_t reg;
	};
	filterSetting _filterSetting;

	union HPF {
		struct {
			uint8_t HPFEL:1;
			uint8_t HPFER:1;
			uint8_t SDAD:1;
			uint8_t SLAD:1;
		} bit;
		uint8_t reg;
	};
	HPF _HPF;

	union LOUTVolume {
		struct {
			uint8_t ATL:8;
		} bit;
		uint8_t reg;
	};
	LOUTVolume _LOUTVolume;

	union ROUTVolume {
		struct {
			uint8_t ATR:8;
		} bit;
		uint8_t reg;
	};
	ROUTVolume _ROUTVolume;
};

#endif /* AUDIOFX_AK4558_H_ */
