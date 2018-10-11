/*
 * fft.h
 *
 *  Created on: Jan 12, 2018
 *      Author: Dean
 */

#ifndef AUDIOFX_FFT_H_
#define AUDIOFX_FFT_H_

#include <Arduino.h>

/**************************************************************
 *  				UTILITY ROM FUNCTIONS
 *************************************************************/

typedef void _twidfftrad2_q31 (complex_q31 *twiddle_table,
                            int fft_size);

static _twidfftrad2_q31* twidfftrad2_q31 = (_twidfftrad2_q31*)0x0401e4bc;

typedef void _twidfftf_q31 (complex_q31 *twiddle_table,
                            int fft_size);

static _twidfftf_q31* twidfftf_q31 = (_twidfftf_q31*)0x0401e3b0;

typedef const complex_q15 _twidfftf_q15_8k_table;
typedef const complex_q31 _twidfftf_q31_4k_table;
typedef const complex_q31 _twidfftrad2_q31_4k_table;

static _twidfftf_q15_8k_table* twidfftf_q15_8k_table = (_twidfftf_q15_8k_table*)0x04079800;
static _twidfftf_q31_4k_table* twidfftf_q31_4k_table = (_twidfftf_q31_4k_table*)0x04073800;
static _twidfftrad2_q31_4k_table* twidfftrad2_q31_4k_table = (_twidfftrad2_q31_4k_table*)0x04073800;

typedef void _cfft_q31 (const complex_q31 *input,
		complex_q31 *output,
		const complex_q31 *twiddle_table,
		int twiddle_stride, int fft_size,
		int* block_exponent, int scale_method);

static _cfft_q31* cfft_q31 = (_cfft_q31*)0x04013358;

typedef void _rfft_q31 (const q31 *input,
		complex_q31 *output,
		const complex_q31 *twiddle_table,
		int twiddle_stride, int fft_size,
		int* block_exponent, int scale_method);

static _rfft_q31* rfft_q31 = (_rfft_q31*)0x0401c2f8;

typedef void _rfftf_q31(const q31 *input,
		complex_q31 *output,
		const complex_q31 *twiddle_table,
		int twiddle_stride, int fft_size);

static _rfftf_q31* rfftf_q31 = (_rfftf_q31*)0x0401c7a0;

typedef void _ifft_q31 (const complex_q31 *input,
		complex_q31 *output,
		const complex_q31 *twiddle_table,
		int twiddle_stride, int fft_size,
		int* block_exponent, int scale_method);

static _ifft_q31* ifft_q31 = (_ifft_q31*)0x040193b0;

typedef void _ifftf_q31 (const complex_q31 *input,
		complex_q31 *output,
		const complex_q31 *twiddle_table,
		int twiddle_stride, int fft_size);

static _ifftf_q31* ifftf_q31 = (_ifftf_q31*)0x04019bd4;


typedef void _rfftf_q15(const q15 *input,
			complex_q15 *output,
			const complex_q15 *twiddle_table,
			int twiddle_stride, int fft_size);

static _rfftf_q15 *rfftf_q15 = (_rfftf_q15*)0x0401c6e4;


/**************************************************************
 *  				END UTILITY ROM FUNCTIONS
 *************************************************************/

#define TWID_SIZE_REAL(x) ((3 * x) / 4)
#define FFT_BIN(num, fs, size) (num*((float)fs/(float)size))

static int block_exponent;

class FFT {
public:
	FFT() : tt(twidfftf_q31_4k_table), tt16(twidfftf_q15_8k_table), _fftMax(4*1024) {}

	~FFT() {}

	//complex input
	void fft(const complex_q31 *in, complex_q31 *out, int size) {
		cfft_q31 (in, out, tt, _fftMax/size, size, &block_exponent, 1); //static scaling
	}

	//run in place
	void fft(complex_q31 *io, int size) { fft(io, io, size); }

	//real input
	void fft(const q31 *in, complex_q31 *out, int size) {
		rfft_q31 (in, out, tt, _fftMax/size, size, &block_exponent, 1); //static scaling
	}

	//ifft complex to real output
	void ifft(complex_q31 *in, q31 *out, int size) {
		ifft_q31(in, in, tt, _fftMax/size, size, &block_exponent, 3); //no scaling

		//take the real component
		for(int i=0; i<size; i++)
			*out++ = in[i].re;
	}


	//ifft complex to complex
	void ifft(const complex_q31 *in, complex_q31 *out, int size) {
		ifft_q31(in, out, tt, _fftMax/size, size, &block_exponent, 3);
	}

	//process in place
	void ifft(complex_q31 *io, int size) { ifft(io, io, size); }

	void combine(complex_q31 *f, int size, int num) {
		//compute IFFT of all
		for(int i=0; i<num; i++)
			ifft(f + size*i, f + size*i, size);

		//compute fft in place for the whole block
		fft(f, size*num);

		//scale back up
		//TODO: figure out why we need to do this
		for(int i=0; i<size*num; i++){
			f[i].re = f[i].re*2;
			f[i].im = f[i].im*2;
		}
	}

	//real input 16 bit
	void rfft_q15(const q15 *in, complex_q15 *out, int size) {
		rfftf_q15(in, out, tt16, _fftMax*2/size, size);
	}

private:
	const complex_q31 *tt;
	const complex_q15 *tt16;
	int _fftMax;
};


#endif /* AUDIOFX_FFT_H_ */
