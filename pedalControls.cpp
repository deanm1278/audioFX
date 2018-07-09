#include "pedalControls.h"
#include "wiring_private.h"

bool PedalControls::begin(){
	memset(&state, 0, sizeof(struct pedalState));

	pinMode(_pin, OUTPUT);
	digitalWrite(_pin, HIGH);
	delay(1);

	uint8_t ch;
	if(_spi->_hw == SPI0)
		ch = SPI0_RXDMA;
	else if(_spi->_hw == SPI1)
		ch = SPI1_RXDMA;
	else if(_spi->_hw == SPI2)
		ch = SPI2_RXDMA;
	else return false;

	DMA[ch]->ADDRSTART.reg = (uint32_t)&state;

	DMA[ch]->CFG.bit.MSIZE = DMA_MSIZE_1_BYTES;
	DMA[ch]->XCNT.reg = sizeof(struct pedalState);
	DMA[ch]->XMOD.reg = 1; //1 byte

	DMA[ch]->CFG.bit.WNR = DMA_CFG_WNR_WRITE_TO_MEM;

	DMA[ch]->CFG.bit.FLOW = DMA_CFG_FLOW_AUTO; //autobuffer mode
	DMA[ch]->CFG.bit.PSIZE = DMA_CFG_PSIZE_1_BYTES;
	DMA[ch]->CFG.bit.EN = DMA_CFG_ENABLE; //enable

	_spi->begin();
	pinPeripheral(_ss, g_APinDescription[_ss].ulPinType);

	//no transmit
	_spi->_hw->TXCTL.reg = 0;

	//enable receive
	_spi->_hw->RXCTL.bit.RDR = 1;
	_spi->_hw->RXCTL.bit.REN = 1;

	_spi->_hw->CTL.bit.MSTR = 0; //slave mode
	_spi->_hw->CTL.bit.SIZE = 0; //1 byte
	_spi->_hw->CTL.bit.EN = 1;

	//signal start
	pinMode(_pin, OUTPUT);
	digitalWrite(_pin, LOW);

	return true;
}

#ifdef SPI_CONTROLS
PedalControls controls(&SPI_CONTROLS, PIN_CONTROLS_SS, PIN_CONTROLS_START);
#endif
