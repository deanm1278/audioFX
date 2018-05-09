#ifndef __ASSEMBLY__
#define __ASSEMBLY__
#endif

#include <bf706_device.h>
#include "audioFX_config.h"

.align 2
.global __mult_q16;
.type __mult_q16, STT_FUNC;
__mult_q16:
	[--SP] = ( R7:2 );
	[--SP] = P0;
	[--SP] = A0.x;
	[--SP] = A0.w;
	[--SP] = A1.x;
	[--SP] = A1.w;
	[--SP] = ASTAT;

	P0 = R0;

	R7 = AUDIO_BUFSIZE;
	LC0 = R7;
	LOOP mult16Loop LC0;
	LOOP_BEGIN mult16Loop;
		R2 = [P0];

		(R5:R4) = ((A1:0) = R2 * R1) (TFU);
		R5 <<= 16;
		R4 >>= 16;

		R2 = R5 | R4;
		[P0] = R2;
		P0 += 4;
	LOOP_END mult16Loop;

	ASTAT = [SP++];
	A1.w = [SP++];
	A1.x = [SP++];
	A0.w = [SP++];
	A0.x = [SP++];
	P0 = [SP++];
	( R7:2 ) = [ SP ++ ];
	RTS;

.align 2
.global __mult_q16_single;
.type __mult_q16_single, STT_FUNC;
__mult_q16_single:
	[--SP] = A0.x;
	[--SP] = A0.w;
	[--SP] = A1.x;
	[--SP] = A1.w;
	[--SP] = ASTAT;


	(R1:R0) = ((A1:0) = R0 * R1) (TFU);
	R1 <<= 16;
	R0 >>= 16;

	R0 = R1 | R0;

	ASTAT = [SP++];
	A1.w = [SP++];
	A1.x = [SP++];
	A0.w = [SP++];
	A0.x = [SP++];
	RTS;

.extern __fm_sine;
.type __fm_sine,STT_OBJECT;

/* This is used if the modulator is an output
 *
 * int _fm_modulate_output(int lastPos, q31 *buf, q16 *cFreq, q31 *mod, q31 *vol);
 */
.align 2
.global __fm_modulate_output;
.type __fm_modulate_output, STT_FUNC;
__fm_modulate_output:
	[--SP] = R4;
	[--SP] = R3;
	R4 = [SP+20]; //mod_buf
	I0 = R4;
	B0 = R4;
	L0 = 0;
	M0 = 4;
	R3 = [SP+24]; //volume_buf
	I3 = R3;
	B3 = R3;
	L3 = 0;
	M3 = 4;

	[--SP] = ( R7:3, P5:4 );
	[--SP] = P1;
	[--SP] = P0;
	[--SP] = A0.x;
	[--SP] = A0.w;
	[--SP] = A1.x;
	[--SP] = A1.w;
	[--SP] = ASTAT;

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
	R0 >>= 10;

	R7 = FM_LOOKUP_INC;

	R4 = AUDIO_BUFSIZE;
	R5 = FM_LOOKUP_MOD;
	LC0 = R4;
	LOOP fmOutputLoop LC0;
	LOOP_BEGIN fmOutputLoop;
		R1 = [I2++M2];
		(R3:R2) = ((A1:0) = R7 * R1) (TFU);

		R2 = PACK(R3.L, R2.H) || R3 = [I0++M0];

		R2 = R2 + R3; //add modulators

		R2 = R2 + R0; //add previous error
		R0 = R2 & R5;

		//pos0 = (q31 * mul) >> 42
		R3 = FM_LOOKUP_MUL;
		R4 = R2 * R3; // (this automatically shifts 31)
		R4 >>= 11;

		//add to plast, plast % 1024
        R4 = R4 + R6;
		R3 = 0x3FF;
		R3 = R4 & R3;
		R6 = R3; //save plast

		R3 <<= 2; //each sample is 4 bytes
		R4 = P4;
		R3 = R3 + R4;
		P5 = R3;
		R4 = [P5]; //val = y[pos0]

		R3 = [I3++M3];
		R4 = R4 * R3 || R3 = [I1];

		//add to output
		R4 = R4 + R3 (S);
		[I1++M1] = R4;

	LOOP_END fmOutputLoop;

	R0 <<= 10;
	R0 = R0 | R6; //return plast and error

	ASTAT = [SP++];
	A1.w = [SP++];
	A1.x = [SP++];
	A0.w = [SP++];
	A0.x = [SP++];
	P0 = [SP++];
	P1 = [SP++];
	( R7:3, P5:4 ) = [ SP ++ ];
	P3 = [SP++];
	P2 = [SP++];
	RTS;

/* This is used if the operator is not an output
 *
 * int _fm_modulate(int lastPos, q31 *buf, q16 *cFreq, q31 *mod, q31 *vol);
 */
.align 2
.global __fm_modulate;
.type __fm_modulate, STT_FUNC;
__fm_modulate:
	[--SP] = R4;
	[--SP] = P3;
	R4 = [SP+20]; //mod_buf
	I0 = R4;
	B0 = R4;
	L0 = 0;
	M0 = 4;
	P3 = [SP+24]; //volume_buf

	[--SP] = ( R7:3, P5:4 );
	[--SP] = P1;
	[--SP] = P0;
	[--SP] = A0.x;
	[--SP] = A0.w;
	[--SP] = A1.x;
	[--SP] = A1.w;
	[--SP] = ASTAT;

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
	R0 >>= 10;

	R7 = FM_LOOKUP_INC;

	R4 = AUDIO_BUFSIZE;
	LC0 = R4;
	LOOP fmLoop LC0;
	LOOP_BEGIN fmLoop;
		R1 = [I2++M2];
		(R3:R2) = ((A1:0) = R7 * R1) (TFU);
		R2 = PACK(R3.L, R2.H) || R3 = [I0++M0];
		R2 <<= 1;

		R2 = R2 + R3;

		R2 = R2 + R0;
		R3 = FM_LOOKUP_MOD;
		R0 = R2 & R3; //add some error bits and save the new error

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

		R6 = R4; //save plast

		R4 <<= 2; //each sample is 4 bytes
		R5 = P4;
		R4 = R4 + R5;
		P5 = R4;
		R4 = [P5]; //val = y[pos0]

		R4 = R4 - R3 (S); //delta = current - last

		//multiply delta by volume
		R3 = [P3];
		P3 += 4;
		R4 = R4 * R3 || R3 = [I1];

		//add delta to output
		R4 = R4 + R3 (S);
		[I1++M1] = R4;

	LOOP_END fmLoop;

	R0 <<= 10;
	R0 = R0 | R6; //return plast and error

	ASTAT = [SP++];
	A1.w = [SP++];
	A1.x = [SP++];
	A0.w = [SP++];
	A0.x = [SP++];
	P0 = [SP++];
	P1 = [SP++];
	( R7:3, P5:4 ) = [ SP ++ ];
	P3 = [SP++];
	P2 = [SP++];
	RTS;
