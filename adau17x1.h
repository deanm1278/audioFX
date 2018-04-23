/*
 * adau17x1.h
 *
 *  Created on: Apr 22, 2018
 *      Author: Dean
 */

#ifndef AUDIOFX_ADAU17X1_H_
#define AUDIOFX_ADAU17X1_H_

#include <stdint.h>
#include "Wire.h"

typedef uint16_t  ADI_DATA_U16;
typedef uint8_t   ADI_REG_TYPE;

#define ADAU1761_I2C_ADDRESS				0x38

#define ADAU17X1_CLOCK_CONTROL				0x4000
#define ADAU17X1_PLL_CONTROL				0x4002
#define ADAU17X1_DIG_DETECT					0x4008
#define ADAU17X1_REC_POWER_MGMT				0x4009
#define ADAU17X1_REC_MIXER_LEFT_0			0x400A
#define ADAU17X1_REC_MIXER_RIGHT_0			0x400C
#define ADAU17X1_MICBIAS					0x4010
#define ADAU17X1_SERIAL_PORT0				0x4015
#define ADAU17X1_SERIAL_PORT1				0x4016
#define ADAU17X1_CONVERTER0					0x4017
#define ADAU17X1_CONVERTER1					0x4018
#define ADAU17X1_LEFT_INPUT_DIGITAL_VOL		0x401a
#define ADAU17X1_RIGHT_INPUT_DIGITAL_VOL	0x401b
#define ADAU17X1_ADC_CONTROL				0x4019
#define ADAU17X1_PLAY_HP_LEFT_VOL			0x4023
#define ADAU17X1_PLAY_HP_RIGHT_VOL			0x4024
#define ADAU17X1_PLAY_POWER_MGMT			0x4029
#define ADAU17X1_DAC_CONTROL0				0x402a
#define ADAU17X1_DAC_CONTROL1				0x402b
#define ADAU17X1_DAC_CONTROL2				0x402c
#define ADAU17X1_SERIAL_PORT_PAD			0x402d
#define ADAU17X1_CONTROL_PORT_PAD0			0x402f
#define ADAU17X1_CONTROL_PORT_PAD1			0x4030
#define ADAU17X1_DSP_SAMPLING_RATE			0x40eb
#define ADAU17X1_SERIAL_INPUT_ROUTE			0x40f2
#define ADAU17X1_SERIAL_OUTPUT_ROUTE		0x40f3
#define ADAU17X1_DSP_ENABLE					0x40f5
#define ADAU17X1_DSP_RUN					0x40f6
#define ADAU17X1_SERIAL_SAMPLING_RATE		0x40f8

class adau17x1 {
public:
	adau17x1(TwoWire *wire=&Wire) : _i2c(wire) {}
	~adau17x1() {}

	bool begin(uint8_t addr=ADAU1761_I2C_ADDRESS);

	void writeReg(uint16_t reg, uint8_t *data, uint8_t count=1);
	void readReg(uint16_t reg, uint8_t *data, uint8_t count=1);

private:
	TwoWire *_i2c;
	uint8_t _addr;

/****** REGISTER DEFINITIONS ******/

	union clockControl {
		struct {
			uint8_t COREN:1;
			uint8_t INFREQ:2;
			uint8_t CLKSRC:1;
		} bit;
		uint8_t reg;
	};
	clockControl _clockControl;

	union PLLControl {
		struct {

			uint16_t M:16;
			uint16_t N:16;

			uint8_t Type:1;
			uint8_t X:2;
			uint8_t R:4;
			uint8_t :1;

			uint8_t PLLEN:1;
			uint8_t Lock:1;
			uint8_t :6;

		} bit;
		uint8_t reg[6];
	};
	PLLControl _PLLControl;

	union digDetect {
		struct {
			uint8_t JDPOL:1;
			uint8_t :3;
			uint8_t JDFUNC:2;
			uint8_t JDDB:2;
		} bit;
		uint8_t reg;
	};
	digDetect _digDetect;

	union recPowerMgmt {
		struct {
			uint8_t :1;
			uint8_t RBIAS:2;
			uint8_t ADCBIAS:2;
			uint8_t MXBIAS:2;
		} bit;
		uint8_t reg;
	};
	recPowerMgmt _recPowerMgmt;

	union recMixerLeft0 {
		struct {
			uint8_t MX1EN:1;
			uint8_t LINNG:3;
			uint8_t LINPG:3;
		} bit;
		uint8_t reg;
	};
	recMixerLeft0 _recMixerLeft0;

	union recMixerLeft1 {
		struct {
			uint8_t MX1AUXG:3;
			uint8_t LDBOOTS:2;
		} bit;
		uint8_t reg;
	};
	recMixerLeft1 _recMixerLeft1;

	union recMixerRight0 {
		struct {
			uint8_t MX2EN:1;
			uint8_t RINNG:3;
			uint8_t RINPG:3;
		} bit;
		uint8_t reg;
	};
	recMixerRight0 _recMixerRight0;

	union recMixerRight1 {
		struct {
			uint8_t MX2AUXG:3;
			uint8_t RDBOOTS:2;
		} bit;
		uint8_t reg;
	};
	recMixerRight1 _recMixerRight1;

	union leftDiffInputVol {
		struct {
			uint8_t LDEN:1;
			uint8_t LDMUTE:1;
			uint8_t LDVOL:6;
		} bit;
		uint8_t reg;
	};
	leftDiffInputVol _leftDiffInputVol;

	union rightDiffInputVol {
		struct {
			uint8_t RDEN:1;
			uint8_t RDMUTE:1;
			uint8_t RDVOL:6;
		} bit;
		uint8_t reg;
	};
	rightDiffInputVol _rightDiffInputVol;

	union recordMicBias {
		struct {
			uint8_t MBIEN:1;
			uint8_t :1;
			uint8_t MBI:1;
			uint8_t MPERF:1;
		} bit;
		uint8_t reg;
	};
	recordMicBias _recordMicBias;

	union ALC0 {
		struct {
			uint8_t ALCSEL:3;
			uint8_t ALCMAX:3;
			uint8_t PGASLEW2:2;
		} bit;
		uint8_t reg;
	};
	ALC0 _ALC0;

	union ALC1 {
		struct {
			uint8_t ALCTARG:4;
			uint8_t ALCHOLD:4;
		} bit;
		uint8_t reg;
	};
	ALC1 _ALC1;

	union ALC2 {
		struct {
			uint8_t ALCDEC:4;
			uint8_t ALCATCK:4;
		} bit;
		uint8_t reg;
	};
	ALC2 _ALC2;

	union ALC3 {
		struct {
			uint8_t NGTHR:5;
			uint8_t NGEN:1;
			uint8_t NGTYP:2;
		} bit;
		uint8_t reg;
	};
	ALC3 _ALC3;

	union serialPort0 {
		struct {
			uint8_t MS:1;
			uint8_t CHPF:2;
			uint8_t LRPOL:1;
			uint8_t BPOL:1;
			uint8_t LRMOD:1;
			uint8_t SPSRS:1;
		} bit;
		uint8_t reg;
	};
	serialPort0 _serialPort0;

	union serialPort1 {
		struct {
			uint8_t LRDEL:2;
			uint8_t MSPB:1;
			uint8_t DATDM:1;
			uint8_t ADTDM:1;
			uint8_t BPF:3;
		} bit;
		uint8_t reg;
	};
	serialPort1 _serialPort1;

	union converter0 {
		struct {
			uint8_t CONVSR:3;
			uint8_t ADOSR:1;
			uint8_t DAOSR:1;
			uint8_t DAPAIR:2;
		} bit;
		uint8_t reg;
	};
	converter0 _converter0;

	union converter1 {
		struct {
			uint8_t ADPAIR:2;
		} bit;
		uint8_t reg;
	};
	converter1 _converter1;

	union ADCControl {
		struct {
			uint8_t ADCEN:2;
			uint8_t INSEL:1;
			uint8_t DMSW:1;
			uint8_t DMPOL:1;
			uint8_t HPF:1;
			uint8_t ADCPOL:1;
		} bit;
		uint8_t reg;
	};
	ADCControl _ADCControl;

	union leftDigitalVol {
		struct {
			uint8_t LADVOL:8;
		} bit;
		uint8_t reg;
	};
	leftDigitalVol _leftDigitalVol;

	union rightDigitalVol {
		struct {
			uint8_t RADVOL:8;
		} bit;
		uint8_t reg;
	};
	rightDigitalVol _rightDigitalVol;

	union playMixerLeft0 {
		struct {
			uint8_t MX3EN:1;
			uint8_t MX3AUXG:4;
			uint8_t MX3LM:1;
			uint8_t MX3RM:1;
		} bit;
		uint8_t reg;
	};
	playMixerLeft0 _playMixerLeft0;

	union playMixerLeft1 {
		struct {
			uint8_t MX3G1:4;
			uint8_t MX3G2:4;
		} bit;
		uint8_t reg;
	};
	playMixerLeft1 _playMixerLeft2;

	union playMixerRight0 {
		struct {
			uint8_t MX4EN:1;
			uint8_t MX4AUXG:4;
			uint8_t MX4LM:1;
			uint8_t MX4RM:1;
		} bit;
		uint8_t reg;
	};
	playMixerRight0 _playMixerRight0;

	union playMixerRight1 {
		struct {
			uint8_t MX4G1:4;
			uint8_t MX4G2:4;
		} bit;
		uint8_t reg;
	};
	playMixerRight1 _playMixerRight1;

	union playLRMixerLeft {
		struct {
			uint8_t MX5EN:1;
			uint8_t MX5G3:2;
		} bit;
		uint8_t reg;
	};
	playLRMixerLeft _playLRMixerLeft;

	union playLRMixerRight {
		struct {
			uint8_t MX6EN:1;
			uint8_t MX6G3:2;
		} bit;
		uint8_t reg;
	};
	playLRMixerRight _playLRMixerRight;

	union playLRMixerMono {
		struct {
			uint8_t MX7EN:1;
			uint8_t MX7:2;
		} bit;
		uint8_t reg;
	};
	playLRMixerMono _playLRMixerMono;

	union playHPLeftVol {
		struct {
			uint8_t HPEN:1;
			uint8_t LHPM:1;
			uint8_t LHPVOL:6;
		} bit;
		uint8_t reg;
	};
	playHPLeftVol _playHPLeftVol;

	union playHPRightVol {
		struct {
			uint8_t HPMODE:1;
			uint8_t RHPM:1;
			uint8_t RHPVOL:6;
		} bit;
		uint8_t reg;
	};
	playHPRightVol _playHPRightVol;

	union lineOutputLeftVol {
		struct {
			uint8_t LOMODE:1;
			uint8_t LOUTM:1;
			uint8_t LOUTVOL:6;
		} bit;
		uint8_t reg;
	};
	lineOutputLeftVol _lineOutputLeftVol;

	union lineOutputRightVol {
		struct {
			uint8_t ROMODE:1;
			uint8_t ROUTM:1;
			uint8_t ROUTVOL:6;
		} bit;
		uint8_t reg;
	};
	lineOutputRightVol _lineOutputRightVol;

	union playMonoOutput {
		struct {
			uint8_t MOMODE:1;
			uint8_t MONOM:1;
			uint8_t MONOVOL:6;
		} bit;
		uint8_t reg;
	};
	playMonoOutput _playMonoOutput;

	union popClickSuppress {
		struct {
			uint8_t :1;
			uint8_t ASLEW:2;
			uint8_t POPLESS:1;
			uint8_t POPMODE:1;
		} bit;
		uint8_t reg;
	};
	popClickSuppress _popClickSuppress;

	union playPowerMgmt {
		struct {
			uint8_t PLEN:1;
			uint8_t PREN:1;
			uint8_t PBIAS:2;
			uint8_t DCBIAS:2;
			uint8_t HPBIAS:2;
		} bit;
		uint8_t reg;
	};
	playPowerMgmt _playPowerMgmt;

	union DACControl0 {
		struct {
			uint8_t DACEN:2;
			uint8_t DEMPH:1;
			uint8_t :1;
			uint8_t DACPOL:1;
			uint8_t DACMONO:2;
		} bit;
		uint8_t reg;
	};
	DACControl0 _DACControl0;

	union DACControl1 {
		struct {
			uint8_t LDAVOL:8;
		} bit;
		uint8_t reg;
	};
	DACControl1 _DACControl1;

	union DACControl2 {
		struct {
			uint8_t RDAVOL:8;
		} bit;
		uint8_t reg;
	};
	DACControl2 _DACControl2;

	union serialPortPad {
		struct {
			uint8_t BCLKP:2;
			uint8_t LRCLKP:2;
			uint8_t DACSDP:2;
			uint8_t ADCSDP:2;
		} bit;
		uint8_t reg;
	};
	serialPortPad _serialPortPad;

	union controlPortPad0 {
		struct {
			uint8_t SDAP:2;
			uint8_t SCLP:2;
			uint8_t CLCHP:2;
			uint8_t CDATP:2;
		} bit;
		uint8_t reg;
	};
	controlPortPad0 _controlPortPad0;

	union controlPortPad1 {
		struct {
			uint8_t SDASTR:1;
		} bit;
		uint8_t reg;
	};
	controlPortPad1 _controlPortPad1;

	union jackDetectPin {
		struct {
			uint8_t :2;
			uint8_t JDP:2;
			uint8_t :1;
			uint8_t JDSTR:1;
		} bit;
		uint8_t reg;
	};
	jackDetectPin _jackDetectPin;

	union dejitterControl {
		struct {
			uint8_t DEJIT:8;
		} bit;
		uint8_t reg;
	};
	dejitterControl _dejitterControl;

	union CyclicRedundancyCheck {
		struct {
			uint32_t CRC:32;
		} bit;
		uint32_t reg;
	};
	CyclicRedundancyCheck _CyclicRedundancyCheck;

	union CRCEnable {
		struct {
			uint8_t CRCEN:1;
		} bit;
		uint8_t reg;
	};
	CRCEnable _CRCEnable;

	union GPIO0PinControl {
		struct {
			uint8_t GPIO0:4;
		} bit;
		uint8_t reg;
	};
	GPIO0PinControl _GPIO0PinControl;

	union GPIO1PinControl {
		struct {
			uint8_t GPIO1:4;
		} bit;
		uint8_t reg;
	};
	GPIO1PinControl _GPIO1PinControl;

	union GPIO2PinControl {
		struct {
			uint8_t GPIO2:4;
		} bit;
		uint8_t reg;
	};
	GPIO2PinControl _GPIO2PinControl;

	union GPIO3PinControl {
		struct {
			uint8_t GPIO3:4;
		} bit;
		uint8_t reg;
	};
	GPIO3PinControl _GPIO3PinControl;

	union watchdogEnable {
		struct {
			uint8_t DOGEN:1;
		} bit;
		uint8_t reg;
	};
	watchdogEnable _watchdogEnable;

	union watchdogValue {
		struct {
			uint32_t DOG:24;
		} bit;
		uint32_t reg;
	};
	watchdogValue _watchdogValue;

	union watchdogError {
		struct {
			uint8_t DOGER;
		} bit;
		uint8_t reg;
	};
	watchdogError _watchdogError;

	union DSPSamplingRateSetting {
		struct {
			uint8_t DSPSR:4;
		} bit;
		uint8_t reg;
	};
	DSPSamplingRateSetting _DSPSamplingRateSetting;

	union serialInputRouteControl {
		struct {
			uint8_t SINRT:4;
		} bit;
		uint8_t reg;
	};
	serialInputRouteControl _serialInputRouteControl;

	union serialOutputRouteControl {
		struct {
			uint8_t SOUTRT:4;
		} bit;
		uint8_t reg;
	};
	serialOutputRouteControl _serialOutputRouteControl;

	union serialDataGPIOPinConfiguration {
		struct {
			uint8_t SDIGP0:1;
			uint8_t SDOGP1:1;
			uint8_t BGP2:1;
			uint8_t LRGP3:1;
		} bit;
		uint8_t reg;
	};
	serialDataGPIOPinConfiguration _serialDataGPIOPinConfiguration;

	union DSPEnable {
		struct {
			uint8_t DSPEN:1;
		} bit;
		uint8_t reg;
	};
	DSPEnable _DSPEnable;

	union DSPRun {
		struct {
			uint8_t DSPRUN:1;
		} bit;
		uint8_t reg;
	};
	DSPRun _DSPRun;

	union DSPSlewModes {
		struct {
			uint8_t LHPSLW:1;
			uint8_t RHPSLW:1;
			uint8_t LOSLW:1;
			uint8_t ROSLW:1;
			uint8_t MOSLW:1;
		} bit;
		uint8_t reg;
	};
	DSPSlewModes _DSPSlewModes;

	union serialPortSamplingRate {
		struct {
			uint8_t SPSR:3;
		} bit;
		uint8_t reg;
	};
	serialPortSamplingRate _serialPortSamplingRate;

	union clockEnable0 {
		struct {
			uint8_t SPPD:1;
			uint8_t SINPD:1;
			uint8_t INTPD:1;
			uint8_t SOUTPD:1;
			uint8_t DECPD:1;
			uint8_t ALCPD:1;
			uint8_t SLEWPD:1;
		} bit;
		uint8_t reg;
	};
	clockEnable0 _clockEnable0;

	union clockEnable1 {
		struct {
			uint8_t CLK0:1;
			uint8_t CLK1:1;
		} bit;
		uint8_t reg;
	};
	clockEnable1 _clockEnable1;
};


#endif /* AUDIOFX_ADAU17X1_H_ */
