/*
 * SigmaStudioFW.h
 *
 *  Created on: Apr 24, 2018
 *      Author: deanm
 */

#ifndef SIGMASTUDIOFW_H_
#define SIGMASTUDIOFW_H_

extern void SIGMA_WRITE_REGISTER_BLOCK(
        uint16_t  devAddress,
        uint16_t  regAddr,
        uint16_t  length,
        uint8_t   *pRegData);

extern void SIGMA_WRITE_DELAY(
        uint16_t  devAddress,
        uint16_t  length,
        uint8_t   *pData);



#endif /* SIGMASTUDIOFW_H_ */
