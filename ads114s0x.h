/*
 * ads114s0x.h
 *
 *  Created on: May 23, 2018
 *      Author: deanm
 */

#ifndef AUDIOFX_ADS114S0X_H_
#define AUDIOFX_ADS114S0X_H_

#include <Arduino.h>
#include "SPI.h"

#define ADS114S0X_ID        0x00
#define ADS114S0X_STATUS    0x01
#define ADS114S0X_INPMUX    0x02
#define ADS114S0X_PGA       0x03
#define ADS114S0X_DATARATE  0x04
#define ADS114S0X_REF       0x05
#define ADS114S0X_IDACMAG   0x06
#define ADS114S0X_IDACMUX   0x07
#define ADS114S0X_VBIAS     0x08
#define ADS114S0X_SYS       0x09
#define ADS114S0X_OFCAL0    0x0B
#define ADS114S0X_OFCAL1    0x0C
#define ADS114S0X_FSCAL0    0x0E
#define ADS114S0X_FSCAL1    0x0F
#define ADS114S0X_GPIODAT   0x10
#define ADS114S0X_GPIOCON   0x11

#define ADS114S0X_COMMAND_NOP       0x00
#define ADS114S0X_COMMAND_WAKEUP    0x02
#define ADS114S0X_COMMAND_POWERDOWN 0x04
#define ADS114S0X_COMMAND_RESET     0x06
#define ADS114S0X_COMMAND_START     0x08
#define ADS114S0X_COMMAND_STOP      0x0A
#define ADS114S0X_COMMAND_SYOCAL    0x16
#define ADS114S0X_COMMAND_SYGCAL    0x17
#define ADS114S0X_COMMAND_SFOCAL    0x19
#define ADS114S0X_COMMAND_RDATA     0x12
#define ADS114S0X_COMMAND_RREG      0x2000
#define ADS114S0X_COMMAND_WREG      0x4000

#define ADS114S0x_DEV_ID_ADS114S08 0x4
#define ADS114S0x_DEV_ID_ADS114S06 0x5

enum {
	ADS114S0X_CONVERSION_MODE_CONTINUOUS = 0,
	ADS114S0X_CONVERSION_MODE_SINGLE,
};

class ads114s0x {
public:
    ads114s0x(int csPin, int rdyPin=-1, SPIClass *spi=&SPI) : _cs(csPin), _spi(spi), _rdy(rdyPin) {

    }
    ~ads114s0x() {}

    bool begin();

    void writeReg(uint8_t reg, uint8_t value);
    uint8_t readReg(uint8_t reg);
    void sendCommand(uint8_t cmd);
    int16_t readChannel(int ch=-1);
    void setConversionMode(uint8_t mode);
    void setChannel(uint8_t ch);
    int readData(bool byCommand = false, bool poll = false);
    void start();
    
private:
    int _cs, _rdy;
    SPIClass *_spi;

    union status {
        struct {
            uint8_t FL_REF_L0:1;
            uint8_t FL_REF_L1:1;
            uint8_t FL_N_RAILN:1;
            uint8_t FL_N_RAILP:1;
            uint8_t FL_P_RAILN:1;
            uint8_t FL_P_RAILP:1;
            uint8_t NRDY:1;
            uint8_t FL_POR:1;
        } bit;
        uint8_t reg;
    };
    status _status;

    union  inpmux {
        struct {
            uint8_t MUXN:4;
            uint8_t MUXP:4;
        } bit;
        uint8_t reg;
    };
    inpmux _inpmux;

    union pga {
        struct {
            uint8_t GAIN:3;
            uint8_t PGA_EN:2;
            uint8_t DELAY:3;
        } bit;
        uint8_t reg;
    };
    pga _pga;

    union datarate {
        struct {
            uint8_t DR:4;
            uint8_t FILTER:1;
            uint8_t MODE:1;
            uint8_t CLK:1;
            uint8_t G_CHOP:1;
        } bit;
        uint8_t reg;
    };
    datarate _datarate;

    union ref {
        struct {
            uint8_t REFCON:2;
            uint8_t REFSEL:2;
            uint8_t NREFN_BUF:1;
            uint8_t NREFP_BUF:1;
            uint8_t FL_REF_EN:2;
        } bit;
        uint8_t reg;
    };
    ref _ref;

    union idacmag {
        struct {
            uint8_t IMAG:4;
            uint8_t :2;
            uint8_t PSW:1;
            uint8_t FL_RAIL_EN:1;
        } bit;
        uint8_t reg;
    };
    idacmag _idacmag;

    union idacmux {
        struct {
            uint8_t I1MUX:4;
            uint8_t I2MUX:4;
        } bit;
        uint8_t reg;
    };
    idacmux _idacmux;

    union vbias {
        struct {
            uint8_t VB_AIN0:1;
            uint8_t VB_AIN1:1;
            uint8_t VB_AIN2:1;
            uint8_t VB_AIN3:1;
            uint8_t VB_AIN4:1;
            uint8_t VB_AIN5:1;
            uint8_t VB_AINC:1;
            uint8_t VB_LEVEL:1;
        } bit;
        uint8_t reg;
    };
    vbias _vbias;

    union sys {
        struct {
            uint8_t SENDSTAT:1;
            uint8_t CRC:1;
            uint8_t TIMEOUT:1;
            uint8_t CAL_SAMP:2;
            uint8_t SYS_MON:3;
        } bit;
        uint8_t reg;
    };
    sys _sys;

    union ofcal {
        struct {
            int16_t OFC:16;
        } bit;
        int16_t reg;
    };
    ofcal _ofcal;

    union fscal {
        struct {
            uint16_t FSC:16;
        } bit;
        uint16_t reg;
    };
    fscal _fscal;

    union gpiodat {
        struct {
            uint8_t DAT:4;
            uint8_t DIR:4;
        } bit;
        uint8_t reg;
    };
    gpiodat _gpiodat;

    union gpiocon {
        struct {
            uint8_t CON:4;
        } bit;
        uint8_t reg;
    };
    gpiocon _gpiocon;

};




#endif /* AUDIOFX_ADS114S0X_H_ */

