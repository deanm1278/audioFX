#ifndef __ASSEMBLY__
#define __ASSEMBLY__
#endif

#include <bf706_device.h>
#include "audioFX_config.h"

.extern __fm_sine;
.type __fm_sine,STT_OBJECT;

//TODO:apply same optimizations as in fm_modulate.S

/* This is used if the operator uses feedback and is an output
 *
 * int _fm_modulate_feedback_output(int lastPos, q31 *buf, q16 *cFreq, q31 *volume_buf, q31 feedbackLevel, q31 *lastFeedback);
 */
.align 2
.global __fm_modulate_feedback_output;
.type __fm_modulate_feedback_output, STT_FUNC;
__fm_modulate_feedback_output:
	[--SP] = P2;
	[--SP] = R3;
	R3 = [SP+20]; //volume_buf
		I3 = R3;
		B3 = R3;
		L3 = 0;
		M3 = 4;
	R3 = [SP+24]; //feedbackLevel
	P2 = [SP+28]; //lastFeedback

	[--SP] = ( R7:4, P5:4 );
	[--SP] = P1;
	[--SP] = P0;

	[--SP] = R3;

	P4.H = __fm_sine;
	P4.L = __fm_sine;

	I1 = R1; //buf
	B1 = R1;
	L1 = 0;
	M1 = 4;

	I2 = R2; //cfreq
		B2 = R2;
		L2 = 0;
		M2 = 4;

	R4 = 0x3FF;
	R6 = R0 & R4;
    R0 = R0 >> 10;

    R7 = FM_LOOKUP_INC;

	R4 = AUDIO_BUFSIZE;
	LC0 = R4;
	LOOP fmFeedbackOutputLoop LC0;
	LOOP_BEGIN fmFeedbackOutputLoop;
		R1 = [I2++M2];
		(R3:R2) = ((A1:0) = R7 * R1) (TFU);

		R2 = PACK(R3.L, R2.H);

		//get feedback parameter
		R3 = [P2];
#if 0
		R4 = 0X28BE60DB; //factor out pi
		R3 = R3 * R4;
#endif
		R5 = [SP];
		R3 = R3 * R5; //multiply by feedback level

		//2*pi( (1/FS)*freq + mod*(1/(2*pi)) )
		R2 = R2 + R3;

		R2 = R2 + R0; //add previous error
		R1 = FM_LOOKUP_MOD;
		R0 = R2 & R1; //save the new error

		R4 = R2 >> 21;

		//add to plast, plast % 1024
		R4 = R4 + R6;
		R3 = 0x3FF;
		R4 = R4 & R3;

		//get old plast
		R3 = R6 << 2; //each is 4 bytes long
		R5 = P4;
		R3 = R3 + R5;
		P5 = R3;
		R3 = [P5]; //R3 = last value

		R6 = R4;

		R4 <<= 2; //each is 4 bytes long
		R5 = P4;
		R4 = R4 + R5;
		P5 = R4;
		R4 = [P5]; //val = y[pos0]

		//moving average to get rid of oscillations
		R4 >>>= 1;
		R2 = R3 >>> 1;
		R4 = R4 + R2;

		R3 = R4 - R3 (S); //feedback delta (current - last)

		//store in feedback param
		[P2] = R3;

		R3 = [I3++M3];
		R4 = R4 * R3 || R3 = [I1];

		//add to output
		R4 = R4 + R3 (S);
		[I1++M1] = R4;

	LOOP_END fmFeedbackOutputLoop;

	R0 <<= 10;
    R0 = R0 | R6; //return plast and error

	SP+=4;

	P0 = [SP++];
	P1 = [SP++];
	( R7:4, P5:4 ) = [ SP ++ ];
	R3 = [SP++];
	P2 = [SP++];
	RTS;

/* This is used if the operator uses feedback
 *
 * int _fm_modulate_feedback(int lastPos, q31 *buf, q16 *cFreq, q31 *volume_buf, q31 feedbackLevel, q31 *lastFeedback);
 */
.align 2
.global __fm_modulate_feedback;
.type __fm_modulate_feedback, STT_FUNC;
__fm_modulate_feedback:
	[--SP] = P2;
	[--SP] = R3;
	R3 = [SP+20]; //volume_buf
		I3 = R3;
		B3 = R3;
		L3 = 0;
		M3 = 4;
	R3 = [SP+24]; //feedbackLevel
	P2 = [SP+28]; //lastFeedback

	[--SP] = ( R7:4, P5:4 );
	[--SP] = P1;
	[--SP] = P0;

	[--SP] = R3;

	P4.H = __fm_sine;
	P4.L = __fm_sine;

	I1 = R1; //buf
		B1 = R1;
		L1 = 0;
		M1 = 4;

	I2 = R2; //cfreq
		B2 = R2;
		L2 = 0;
		M2 = 4;

	R4 = 0x3FF;
	R6 = R0 & R4;
    R0 = R0 >> 10;

    R7 = FM_LOOKUP_INC;

	R4 = AUDIO_BUFSIZE;
	LC0 = R4;
	LOOP fmFeedbackLoop LC0;
	LOOP_BEGIN fmFeedbackLoop;
		R1 = [I2++M2];
		(R3:R2) = ((A1:0) = R7 * R1) (TFU);
		R2 = PACK(R3.L, R2.H);

		//get feedback parameter
		R3 = [P2];
		R5 = [SP];
		R3 = R3 * R5; //multiply by feedback level

		//2*pi( (1/FS)*freq + mod*(1/(2*pi)) )
		R2 = R2 + R3;

		R2 = R2 + R0; //add previous error
		R1 = FM_LOOKUP_MOD;
		R0 = R2 & R1; //save the new error

		R4 = R2 >> 21;

		//add to plast, plast % 1024
		R4 = R4 + R6;
		R3 = 0x3FF;
		R4 = R4 & R3;

		//get old plast
		R3 = R6 << 2; //each is 4 bytes long
		R5 = P4;
		R3 = R3 + R5;
		P5 = R3;
		R3 = [P5]; //R3 = last value

		R6 = R4; //save new plast

		R4 <<= 2; //each is 4 bytes long
		R5 = P4;
		R4 = R4 + R5;
		P5 = R4;
		R4 = [P5]; //val = y[pos0]

		//moving average to get rid of oscillations
		R4 >>>= 1;
		R2 = R3 >>> 1;
		R4 = R4 + R2;

		R3 = R4 - R3 (S); //feedback delta (current - last)

		//store in feedback param
		[P2] = R3;

		//multiply delta by volume
		R4 = [I3++M3];
		R3 = R4 * R3 || R4 = [I1];
		R3 <<= 1; //TODO: figure out where we might be losing this factor of 2?

		//add delta to output
		R4 = R4 + R3 (S);
		[I1++M1] = R4;

	LOOP_END fmFeedbackLoop;

	R0 <<= 10;
    R0 = R0 | R6; //return plast and error

	SP+=4;

	P0 = [SP++];
	P1 = [SP++];
	( R7:4, P5:4 ) = [ SP ++ ];
	R3 = [SP++];
	P2 = [SP++];
	RTS;
