#ifndef __ASSEMBLY__
#define __ASSEMBLY__
#endif

#include <bf706_device.h>
#include "audioFX_config.h"

.extern __fm_sine;
.type __fm_sine,STT_OBJECT;

/* This is used if the operator uses feedback and is an output
 *
 * int _fm_modulate_feedback_output(int lastPos, q31 *buf, q16 *cFreq, q31 *volume_buf, q31 feedbackLevel, q31 *lastFeedback);
 */
.align 2
.global __fm_modulate_feedback_output;
.type __fm_modulate_feedback_output, STT_FUNC;
__fm_modulate_feedback_output:
	[--SP] = P2;
	[--SP] = P3;
	[--SP] = R3;
	P3 = [SP+24]; //volume_buf
	R3 = [SP+28]; //feedbackLevel
	P2 = [SP+32]; //lastFeedback

	[--SP] = ( R7:4, P5:4 );
	[--SP] = P1;
	[--SP] = P0;
	[--SP] = A0.x;
	[--SP] = A0.w;
	[--SP] = A1.x;
	[--SP] = A1.w;
	[--SP] = ASTAT;

	[--SP] = R3;

	P4.H = __fm_sine;
	P4.L = __fm_sine;

	P0 = R1; //buf
	P1 = R2; //cfreq

	R6 = R0;
	R0 = 0;

	R7 = AUDIO_BUFSIZE;
	LC0 = R7;
	LOOP fmFeedbackOutputLoop LC0;
	LOOP_BEGIN fmFeedbackOutputLoop;
		R1 = [P1];
		P1 += 4;

		R7 = R1 << 16; //fraction
		R5 = R1 >> 16; //integer
		R4 = FM_LOOKUP_INC;
		R3 = R4 * R7;
		(R5:R4) = ((A1:0) = R4 * R5) (TFU);

		R2 = R4 + R3; // R2 = freq*t

		//get feedback parameter
		R3 = [P2];
		R4 = 0X28BE60DB; //factor out pi
		R3 = R3 * R4;
		R5 = [SP];
		R3 = R3 * R5; //multiply by feedback level

		//2*pi( (1/FS)*freq + mod*(1/(2*pi)) )
		R2 = R2 + R3;

		R2 = R2 + R0; //add previous error
		R1 = FM_LOOKUP_MOD;
		R0 = R2 & R1; //save the new error

		//pos0 = (q31 * mul) >> 42
		R3 = FM_LOOKUP_MUL;
		R4 = R2 * R3; // (this automatically shifts 31)
		R4 >>= 11;

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

		R3 = R4 - R3; //feedback delta (current - last)

		//store in feedback param
		[P2] = R3;

		//multiply by volume
		R3 = [P3];
		P3 += 4;
		R4 = R4 * R3;

		//add to output
		R3 = [P0];
		R4 = R4 + R3;
		[P0] = R4;
		P0 += 4;

	LOOP_END fmFeedbackOutputLoop;

	R0 = R6;

	SP+=4;

	ASTAT = [SP++];
	A1.w = [SP++];
	A1.x = [SP++];
	A0.w = [SP++];
	A0.x = [SP++];
	P0 = [SP++];
	P1 = [SP++];
	( R7:4, P5:4 ) = [ SP ++ ];
	R3 = [SP++];
	P3 = [SP++];
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
	[--SP] = P3;
	[--SP] = R3;
	P3 = [SP+24]; //volume_buf
	R3 = [SP+28]; //feedbackLevel
	P2 = [SP+32]; //lastFeedback

	[--SP] = ( R7:4, P5:4 );
	[--SP] = P1;
	[--SP] = P0;
	[--SP] = A0.x;
	[--SP] = A0.w;
	[--SP] = A1.x;
	[--SP] = A1.w;
	[--SP] = ASTAT;

	[--SP] = R3;

	P4.H = __fm_sine;
	P4.L = __fm_sine;

	P0 = R1; //buf
	P1 = R2; //cfreq

	R6 = R0;
	R0 = 0;

	R7 = AUDIO_BUFSIZE;
	LC0 = R7;
	LOOP fmFeedbackLoop LC0;
	LOOP_BEGIN fmFeedbackLoop;
		R1 = [P1];
		P1 += 4;

		R7 = R1 << 16; //fraction
		R5 = R1 >> 16; //integer
		R4 = FM_LOOKUP_INC;
		R3 = R4 * R7;
		(R5:R4) = ((A1:0) = R4 * R5) (TFU);

		R2 = R4 + R3; // R2 = freq*t

		//get feedback parameter
		R3 = [P2];
		R4 = 0X28BE60DB; //factor out pi
		R3 = R3 * R4;
		R5 = [SP];
		R3 = R3 * R5; //multiply by feedback level

		//2*pi( (1/FS)*freq + mod*(1/(2*pi)) )
		R2 = R2 + R3;

		R2 = R2 + R0; //add previous error
		R1 = FM_LOOKUP_MOD;
		R0 = R2 & R1; //save the new error

		//pos0 = (q31 * mul) >> 42
		R3 = FM_LOOKUP_MUL;
		R4 = R2 * R3; // (this automatically shifts 31)
		R4 >>= 11;

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

		//multiply the delta by volume
		R4 = [P3];
		P3 += 4;
		R3 = R4 * R3;

		//add the delta to the return buffer
		R4 = [P0];
		R4 = R4 + R3;
		[P0] = R4;
		P0 += 4;

	LOOP_END fmFeedbackLoop;

	R0 = R6;

	SP+=4;

	ASTAT = [SP++];
	A1.w = [SP++];
	A1.x = [SP++];
	A0.w = [SP++];
	A0.x = [SP++];
	P0 = [SP++];
	P1 = [SP++];
	( R7:4, P5:4 ) = [ SP ++ ];
	R3 = [SP++];
	P3 = [SP++];
	P2 = [SP++];
	RTS;
