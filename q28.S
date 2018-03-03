/*
 * _mult_q28xq16_mod(q28, q16);
 */

#ifndef __ASSEMBLY__
#define __ASSEMBLY__
#endif

#include <bf706_device.h>
#include "audioFX_config.h"

.align 2
.global __mult_q16;
.type __mult_q16, STT_FUNC;
__mult_q16:
	LINK 0;
	[--SP] = A0.x;
	[--SP] = A0.w;
	[--SP] = A1.x;
	[--SP] = A1.w;

	A1 = R0.L * R1.L (FU);
	A1 = A1 >> 16;
	A0 = R0.H * R1.H, A1 += R0.H * R1.L (FU);
	A1 += R1.H * R0.L (FU);
	A0 = A0 << 16;
	R0 = (A0 += A1);

	A1.w = [SP++];
	A1.x = [SP++];
	A0.w = [SP++];
	A0.x = [SP++];
	UNLINK;
	RTS;


.align 2
.global __mult_q28xq28;
.type __mult_q28xq28, STT_FUNC;
__mult_q28xq28:
	[--SP] = A0.x;
	[--SP] = A0.w;
	[--SP] = A1.x;
	[--SP] = A1.w;

	//TODO: this loses 3 bits of precision.. fix
	A1 = R0.L * R1.L (FU);
	A1 = A1 >> 16;
	A0 = R0.H * R1.H, A1 += R0.H * R1.L (M);
	A1 += R1.H * R0.L (M);
	A1 = A1 >>> 15;
	R0 = (A0 += A1);

	R0 = R0 << 3;

	A1.w = [SP++];
	A1.x = [SP++];
	A0.w = [SP++];
	A0.x = [SP++];
	RTS;


.extern __fm_sine;
.type __fm_sine,STT_OBJECT;

/*
 * q31 _fm_modulate(q28 t, q31 *buf, q16 *cFreq, q31 *mod, q31 *vol, q31 gain);
 */
.align 2
.global __fm_modulate;
.type __fm_modulate, STT_FUNC;
__fm_modulate:
	[--SP] = P2;
	[--SP] = P3;
	[--SP] = R3;
	P2 = [SP+24]; //mod_buf
	P3 = [SP+28]; //volume_buf
	R3 = [SP+32]; //gain

	[--SP] = ( R7:3, P5:4 );
	[--SP] = P1;
	[--SP] = P0;
	[--SP] = A0.x;
	[--SP] = A0.w;
	[--SP] = A1.x;
	[--SP] = A1.w;
	[--SP] = ASTAT;

	[--SP] = R3;

	P0 = R1; //buf
	P1 = R2; //cfreq
	P4.H = __fm_sine;
	P4.L = __fm_sine;

	R6 = FM_LOOKUP_MASK;

	R7 = AUDIO_BUFSIZE;
	LC0 = R7;
	LOOP fmLoop LC0;
	LOOP_BEGIN fmLoop;
		R1 = [P1];
		R7 = R1 << 12;
		P1 += 4;

		R5 = R1 >> 16;
		R3 = 0xFFF0;
		R5 = R5 & R3;

		(R3:R2) = ((A1:0) = R0 * R7) (TFU);
		(R5:R4) = ((A1:0) = R0 * R5) (TFU);

		R3 <<= 4;

		R2 = R3 + R4; // R2 = pi*freq*t
		R2 <<= 1; // R2 = 2*pi*freq*t

		R3 = [P2];
		P2 += 4;
#if 0
		// scale down mods by pi and to q28
		R4 = 0XA2F9836;
		R3 = R3 * R4;
#endif
		R2 = R2 + R3;

		R2 = R2 & R6; //mask off anything greater than 2pi

		//pos0 = (q28 * mul) >> 38
		R3 = FM_LOOKUP_MUL;
		R4 = R2 * R3; // (this automatically shifts 31)
		R4 >>= 7;

		R4 <<= 3; //first 4 bytes are the position, next 4 are the derivative
		R5 = P4;
		R4 = R4 + R5;
		P5 = R4;
		R4 = [P5]; //val = y[pos0]

#if 0
		//This doesn't seem to be giving any extra resolution
		R5 = FM_LOOKUP_MOD;
		R5 = R2 & R5;
		R3 = [P5 + 4];
		R5 <<= 12;
		R5 = R5 * R3;
		R4 = R4 + R5;
#endif

		//multiply by volume
		R3 = [P3];
		P3 += 4;
		R4 = R4 * R3;

		//multiply by gain
		R3 = [SP];
		R4 = R4 * R3;

		//add to output
		R3 = [P0];
		R4 = R4 + R3;
		[P0] = R4;
		P0 += 4;

		//increment time step
		R7 = FM_LOOKUP_INC;
		R0 = R0 + R7;

	LOOP_END fmLoop;

	SP+=4;

	ASTAT = [SP++];
	A1.w = [SP++];
	A1.x = [SP++];
	A0.w = [SP++];
	A0.x = [SP++];
	P0 = [SP++];
	P1 = [SP++];
	( R7:2, P5:4 ) = [ SP ++ ];
	R7 = [SP++];
	P3 = [SP++];
	P2 = [SP++];
	RTS;
