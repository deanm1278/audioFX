/*
 * _mult_q28xq16_mod(q28, q16);
 */

#ifndef __ASSEMBLY__
#define __ASSEMBLY__
#endif

#include <bf706_device.h>

.align 2
.global __mult_q28xq28;
.type __mult_q28xq28, STT_FUNC;
__mult_q28xq28:
	LINK 0;
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

	UNLINK;
	RTS;

.align 2
.global __mult_q28xq16_mod;
.type __mult_q28xq16_mod, STT_FUNC;
__mult_q28xq16_mod:
	LINK 0;
	[--SP] = ( R7:2 );
	[--SP] = A0.x;
	[--SP] = A0.w;
	[--SP] = A1.x;
	[--SP] = A1.w;

	R6 = R0 >> 12; //q28_hi = q28 >> 12

	R7.L = 0x0FFF;
	R7.H = 0;
	R5 = R0 & R7; //q28_lo = (q28 & 0x00000FFF)

	R4 = R1 << 12;
	R7.L = LO(0x1FFFFFFF);
	R7.H = HI(0x1FFFFFFF);
	R4 = R4 & R7; //q16_lo = (q16 << 12) & 0x1FFFFFFF

	R3 = R1 >> 16; //q16_hi = (q16 >> 16)

	//TODO: this may not work... may lose integer
	R6 *= R1;
	R6 =  R6 >> 4; //acc1 = ((q28_hi * q16) >> 4)

	//TODO: this loses 3 bits of precision.. fix
	A1 = R4.L * R5.L (FU);
	A1 = A1 >> 16;
	A0 = R4.H * R5.H, A1 += R4.H * R5.L (M);
	A1 += R5.H * R4.L (M);
	A1 = A1 >>> 15;
	R2 = (A0 += A1);

	R2 = R2 << 3; //acc2 = q28_mul(q16_lo, q28_lo)

	R5 *= R3;
	R2 = R2 + R5; //acc2 = acc2 + (q28_lo * q16_hi)

	R0 = R2 + R6; //return acc1 + acc2

	A1.w = [SP++];
	A1.x = [SP++];
	A0.w = [SP++];
	A0.x = [SP++];
	( R7 : 2 ) = [ SP ++ ];
	UNLINK;
	RTS;
