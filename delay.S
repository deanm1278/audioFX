#ifndef __ASSEMBLY__
#define __ASSEMBLY__
#endif

#include <bf706_device.h>

/*
 * void _delay_push(struct delayLine *line, q31 *buf, uint32_t num);
 */

.align 2
.global __delay_push;
.type __delay_push, STT_FUNC;
__delay_push:
	[--SP] = ( R7:6, P5:4 );

	P4 = R0;
	R7 = [P4];
	I0 = R7; //index = line.head
	P4 += 4;
	R7 = [P4]; //line.data
	P4 += 4;
	R6 = [P4]; //line.size
	I1 = R1; //buf
	B1 = R1;
	L1 = 0;

	B0 = R7; //base = line.data

	R6 <<= 2;
	L0 = R6; //length = line.size << 2

	M0 = 4; //modifier = sizeof(q31)
	M1 = 4;

	LC0 = R2; //size
	LOOP delayPushLoop LC0;
	LOOP_BEGIN delayPushLoop;
		R6 = [I1++M1];
		[I0++] = R6;
	LOOP_END delayPushLoop;

	//store back the current head value
	P4 = R0;
	R1 = I0;
	[P4] = R1;

	( R7:6, P5:4 ) = [ SP ++ ];
	RTS;

__delay_setup:
    I1 = R1; //data will be output here
    B1 = R1;
    L1 = 0;
    M1 = 4;

    P4 = R0;
    P3 = R0;
    R5 = [P4]; //R5 = tap->parent

    //load up circular address
    P4 = R5;
    P4 += 4;
    R7 = [P4]; //line.data
    P4 += 4;
    R6 = [P4]; //line.size

    B0 = R7; //base = line.data

    R6 <<= 2;
    L0 = R6; //length = line.size << 2

    P4 = R0;
    P4 += 4;
    R7 = [P4];
    I0 = R7; //I0 = tap->dptr;
    P4 += 4;
    R6 = [P4]; //R6 = tap->currentOffset
    P4 += 4;
    R5 = [P4]; //R5 = tap->roc
    P4 += 4;
    R7 = [P4]; //R7 = tap->top
    P4 += 4;
    R3 = [P4]; //R3 = tap->direction
    P4 += 4;
    R4 = [P4]; //R4 = err
    RTS;

/*
 * void _fir(struct fir *f, q31 *buf, uint32_t num);
 */

.align 2
.global __fir;
.type __fir, STT_FUNC;
__fir:
	[--SP] = ( R7:3, P5:2 );

    I2 = R1; //data will be output here
    B2 = R1;
    L2 = 0;
    M2 = 4;

	P4 = R0;
	P3 = R0;
	R5 = [P4]; //R5 = fir->line

	//load up circular address
	P4 = R5;
	P4 += 4;
	R7 = [P4]; //line.data
	P4 += 4;
	R6 = [P4]; //line.size

	B0 = R7; //base = line.data
	B1 = R7;

	R6 <<= 2;
    L0 = R6; //length = line.size << 2
    L1 = R6;

	P4 = R0;
    P4 += 4;
    R7 = [P4];
    I0 = R7; //I0 = fir->dptr;

    P4 += 4;
    R4 = [P4]; //R4 = fir->coeffs

    P4 += 4;
    R7 = [P4]; //R7 = fir->num;

    M0 = 4; //modifier = sizeof(q31)
    M1 = -4; //modifier1 = -sizeof(q31)

    LC0 = R2; //num
	LOOP firLoop LC0;
	LOOP_BEGIN firLoop;
		R6 = I0;
		I1 = R6; //I1 = dptr

		R6 = 0 (X);
		P2 = R4;

		LC1 = R7; //fir->num
		LOOP firInnerLoop LC1;
		LOOP_BEGIN firInnerLoop;
			R5 = [I1++M1] || R3 = [P2];
			P2 += 4;
			R2 = R5 * R3;
			R6 = R6 + R2;

		LOOP_END firInnerLoop;

		I0 += M0 || [I2++M2] = R6;
	LOOP_END firLoop;

	//save back variables
    P3 += 4;
    R1 = I0;
    [P3] = R1; //tap->dptr

	( R7:3, P5:2 ) = [ SP ++ ];
	RTS;

/*
 * void _biquad(struct biquad *b, q31 *buf, uint32_t num);
 */

.align 2
.global __biquad;
.type __biquad, STT_FUNC;
__biquad:
    [--SP] = ( R7:3, P5:2 );

    I2 = R1; //data will be output here
    B2 = R1;
    L2 = 0;

    M0 = 4;
    M1 = -4;
    M2 = 12;
    M3 = 8;

    P4 = R0;
    P3 = R0;
    R5 = [P4]; //R5 = biquad->output

    //load up output circular address
    P4 = R5;
    P4 += 4;
    R7 = [P4]; //biquad->output.data
    P4 += 4;
    R6 = [P4]; //biquad->output.size

    B0 = R7; //base = biquad->output.data

    R6 <<= 2;
    L0 = R6; //length = biquad->output.size << 2

    P4 = R0;
    P4 += 4;
    R5 = [P4]; //R5 = biquad->input

    //load up input circular address
    P4 = R5;
    P4 += 4;
    R7 = [P4]; //biquad->input.data
    P4 += 4;
    R6 = [P4]; //biquad->input.size

    B1 = R7; //base = biquad->input.data

    R6 <<= 2;
    L1 = R6; //length = biquad->input.size << 2

    P4 = R0;
    P4 += 8;
    R7 = [P4];
    I0 = R7; //I0 = biquad->outptr;
    P4 += 4;
    R7 = [P4];
    I1 = R7; //I1 = biquad->inptr;

    P4 += 4;
    R3 = [P4]; P4 += 4; //R3 = a1
    R4 = [P4]; P4 += 4; //R4 = a2
    R5 = [P4]; P4 += 4; //R5 = b0
    R6 = [P4]; P4 += 4; //R6 = b1
    R7 = [P4]; P4 += 4; //R7 = b2

    LC0 = R2; //num
    LOOP biquadLoop LC0;
    LOOP_BEGIN biquadLoop;
    	R1 = [I1++M1];
        (A1:0) = R1 * R5 || R1 = [I1++M1]; //b0 * x[i]
        (A1:0) += R1 * R6 || R1 = [I1]; //b1 * x[i-1]
        (A1:0) += R1 * R7 || I1+=M2 || R1 = [I0++M1]; //b2 * x[i-2]

        (A1:0) -= R1 * R3 || R1 = [I0++M3]; //a1 * y[i-1]
        (R1:R0) = ((A1:0) -= R1 * R4); //a2 * y[i-2]
        R1 <<= 3; //q28 to q31
        [I0] = R1;
        [I2++M0] = R1;
    LOOP_END biquadLoop;

    //save back variables
    P3 += 8;
    R1 = I0;
    [P3] = R1; //biquad->outptr

    P3 += 4;
    R1 = I1;
    [P3] = R1; //biquad->inptr

    ( R7:3, P5:2 ) = [ SP ++ ];
    RTS;

/*
 * void __delay_pop(struct delayTap *tap, q31 *buf, uint32_t num);
 */

.align 2
.global __delay_pop;
.type __delay_pop, STT_FUNC;
__delay_pop:
    LINK 0
	[--SP] = ( R7:3, P5:3 );

    CALL __delay_setup;

    M0 = 4; //modifier = sizeof(q31)

	LC0 = R2; //num
	LOOP delayPopLoop LC0;
	LOOP_BEGIN delayPopLoop;
		R6 = [I0++M0];
		[I1++M1] = R6;
	LOOP_END delayPopLoop;

	//save back variables
    P3 += 4;
    R1 = I0;
    [P3] = R1; //tap->dptr

	( R7:3, P5:3 ) = [ SP ++ ];
	UNLINK;
	RTS;


/*
 * void _delay_modulate(struct delayTap *tap, q31 *buf, uint32_t num);
 */

.align 2
.global __delay_modulate;
.type __delay_modulate, STT_FUNC;
__delay_modulate:
    LINK 0
	[--SP] = ( R7:3, P5:3 );

    CALL __delay_setup;

	LC0 = R2; //num
	LOOP delayModulateLoop LC0;
	LOOP_BEGIN delayModulateLoop;
		R2 = R5 * R3; //roc * direction

		//add to error
		R4 = R4 + R2 (S);
		R2 = R4 >>> 16; //integer portion

		R0 = R2 - R6; //current integer portion - last integer portion
		R6 = R2;
		R0 <<= 2; //4 bytes per q31
		R0 = -R0; //negate since we're moving away from buffer head
		M0 = R0;

		I0 += M0;
		R1 = [I0]; //*dptr

		//get previous sample
		M0 = -4;
		I0 += M0;
		M0 = 8; //move back to where it was
		R0 = [I0++M0]; //*nptr

		//interpolate the delay line
		R2 = R0 - R1;
		R0 = R4 << 15; //convert err to q31
		BITCLR (R0, 31); //clear sign bit
		R0 = R0 * R2;
		R0 = R1 + R0;
		[I1++M1] = R0;

		R2 = 0x80000000;
		CC = R6 == R7;
		IF CC R3 = R2;

		R1 = 0 (X);
		R2 = 0x7FFFFFFF;
		CC = R6 == R1;
		IF CC R3 = R2;

	LOOP_END delayModulateLoop;

	//save back variables
	P3 += 4;
	R1 = I0;
	[P3] = R1; //tap->dptr
	P3 += 4;
	R6 = R4 >> 16;
	[P3] = R6; //tap->currentOffset
	P3 += 12;
	[P3] = R3; //tap->direction
	P3 += 4;
	[P3] = R4; //tap->err

	( R7:3, P5:3 ) = [ SP ++ ];
	UNLINK
	RTS;

.extern _pitch_shift_table;
.type _pitch_shift_table,STT_OBJECT;

/*
 * void _delay_pitch_shift_down(struct delayTap *tap, q31 *buf, uint32_t num);
 */

.align 2
.global __delay_pitch_shift_down;
.type __delay_pitch_shift_down, STT_FUNC;
__delay_pitch_shift_down:
    LINK 0
    [--SP] = ( R7:3, P5:2 );

    CALL __delay_setup;

    LC0 = R2; //num
    LOOP delayPitchShiftDownLoop LC0;
    LOOP_BEGIN delayPitchShiftDownLoop;
        R2 = R5; //direction is positive

        //add to error
        R4 = R4 + R2 (S);
        R0 = 0x07FFFFFF; //q16 rep of 2047.9999

        CC = R4 < R0;
        IF CC JUMP pitch_shift_down_cont;
        //otherwise we have rolled over
        R1 = 2048;
        R6 = R6 - R1;
        R4 = R4 & R0; //modulus

pitch_shift_down_cont:
        R2 = R4 >>> 16; //integer portion
        R0 = R2 - R6; //current integer portion - last integer portion

        R6 = R2;
        R0 <<= 2; //4 bytes per q31
        R0 = -R0; //negate since we're moving away from buffer head
        M0 = R0;

        I0 += M0;
        R1 = [I0]; //*dptr

        //get previous sample
        M0 = -4;
        I0 += M0;
        M0 = 8; //move back to where it was
        R0 = [I0++M0]; //*nptr

        //interpolate the delay line
        R2 = R0 - R1;
        R0 = R4 << 15; //convert err to q31
        BITCLR (R0, 31); //clear sign bit
        R0 = R0 * R2;
        R0 = R1 + R0;

        //TODO: interpolate the pitch shift table as well
        P2.H = _pitch_shift_table;
        P2.L = _pitch_shift_table;
        R7 = R6 << 2;
        R1 = P2;
        R7 = R1 + R7;
        P2 = R7;
        R7 = [P2];
        R0 = R0 * R7; //multiply by the correct volume

        [I1++M1] = R0; //save and increment output pointers

    LOOP_END delayPitchShiftDownLoop;

    //save back variables
    P3 += 4;
    R1 = I0;
    [P3] = R1; //tap->dptr
    P3 += 4;
    R6 = R4 >> 16;
    [P3] = R6; //tap->currentOffset
    P3 += 16;
    [P3] = R4; //tap->err

    ( R7:3, P5:2 ) = [ SP ++ ];
    UNLINK
    RTS;

/*
 * void _delay_pitch_shift_up(struct delayTap *tap, q31 *buf, uint32_t num);
 */

.align 2
.global __delay_pitch_shift_up;
.type __delay_pitch_shift_up, STT_FUNC;
__delay_pitch_shift_up:
    LINK 0
    [--SP] = ( R7:3, P5:2 );

    CALL __delay_setup;

    LC0 = R2; //num
    LOOP delayPitchShiftUpLoop LC0;
    LOOP_BEGIN delayPitchShiftUpLoop;
        R2 = R5;

        //add to error
        R4 = R4 - R2; //direction is negative
        R0 = 0x07FFFFFF; //q16 rep of 2047.9999

        CC = BITTST(R4, 31);
        IF !CC JUMP pitch_shift_up_cont;
        //otherwise we have rolled over
        R1 = 2048;
        R6 = R1 + R6;
        R4 = R0 + R4;

pitch_shift_up_cont:
        R2 = R4 >>> 16; //integer portion
        R0 = R2 - R6; //current integer portion - last integer portion

        R6 = R2;
        R0 <<= 2; //4 bytes per q31
        R0 = -R0; //negate since we're moving away from buffer head
        M0 = R0;

        I0 += M0;
        R1 = [I0]; //*dptr

        //get previous sample
        M0 = -4;
        I0 += M0;
        M0 = 8; //move back to where it was
        R0 = [I0++M0]; //*nptr

        //interpolate the delay line
        R2 = R0 - R1;
        R0 = R4 << 15; //convert err to q31
        BITCLR (R0, 31); //clear sign bit
        R0 = R0 * R2;
        R0 = R1 + R0;

        //TODO: interpolate the pitch shift table as well
        P2.H = _pitch_shift_table;
        P2.L = _pitch_shift_table;
        R7 = R6 << 2;
        R1 = P2;
        R7 = R1 + R7;
        P2 = R7;
        R7 = [P2];
        R0 = R0 * R7; //multiply by the correct volume

        [I1++M1] = R0; //save and increment output pointers

    LOOP_END delayPitchShiftUpLoop;

    //save back variables
    P3 += 4;
    R1 = I0;
    [P3] = R1; //tap->dptr
    P3 += 4;
    R6 = R4 >> 16;
    [P3] = R6; //tap->currentOffset
    P3 += 16;
    [P3] = R4; //tap->err

    ( R7:3, P5:2 ) = [ SP ++ ];
    UNLINK
    RTS;
