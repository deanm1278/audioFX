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
	P5 = R1; //buf

	B0 = R7; //base = line.data

	R6 <<= 2;
	L0 = R6; //length = line.size << 2

	M0 = 4; //modifier = sizeof(q31)

	LC0 = R2; //size
	LOOP delayPushLoop LC0;
	LOOP_BEGIN delayPushLoop;
		R6 = [P5];
		[I0++] = R6;
		P5+=4;
	LOOP_END delayPushLoop;

	//store back the current head value
	P4 = R0;
	R1 = I0;
	[P4] = R1;

	( R7:6, P5:4 ) = [ SP ++ ];
	RTS;

__delay_setup:
    P5 = R1; //data will be output here

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
    R4 = [P4]; //R4 = tap->top
    [--SP] = R4;
    P4 += 4;
    R3 = [P4]; //R3 = tap->direction
    P4 += 4;
    R4 = [P4]; //R4 = err
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
		[P5] = R0;
		P5 += 4;

		//pop tap->top off the stack
		R0 = [SP];

		CC = R6 == R0;
		IF CC JUMP dir_neg;

		R1 = 0 (X);
		CC = R6 == R1;
		IF CC JUMP dir_pos;

		JUMP delay_loop_continue;

dir_neg:
		R3 = 0x80000000; //dir = _F(-1)
		JUMP delay_loop_continue;
dir_pos:
		R3 = 0x7FFFFFFF;

delay_loop_continue:
		NOP;

	LOOP_END delayModulateLoop;

	SP+=4;

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

        [P5] = R0; //save and increment output pointers
        P5 += 4;

    LOOP_END delayPitchShiftDownLoop;

    SP+=4;

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

        [P5] = R0; //save and increment output pointers
        P5 += 4;

    LOOP_END delayPitchShiftUpLoop;

    SP+=4;

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
