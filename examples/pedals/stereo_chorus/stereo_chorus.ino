#include "audioFX.h"
#include "adau17x1.h"
#include "Adafruit_ADS1015.h"
#include "fm.h"

#define ADC_SCALE_FACTOR 1048576
#define RATE_SCALE_FACTOR 200

/* the maximum number of samples the chorus voice will be delayed */
#define DELAY_MAX_ORDER (10)
#define MAX_DELAY (1UL << DELAY_MAX_ORDER)
#define DELAY_BUFSIZE ((MAX_DELAY/AUDIO_BUFSIZE) * AUDIO_BUFSIZE)

q31 dataL[DELAY_BUFSIZE], dataR[DELAY_BUFSIZE];

typedef struct {
    q31 *head;
    q31 count;
    uint32_t currentOffset;
    q31 *dptr;
    q31 *data;
} liteBuf;

static liteBuf bufL = {dataL, 0, 0, dataL, dataL};
static liteBuf bufR = {dataR, 0, 0, dataR, dataR};

static RAMB q31 lfoDataL[AUDIO_BUFSIZE], lfoDataR[AUDIO_BUFSIZE];
LFO<q31> lfoLeft( _F16(0.5) );
LFO<q31> lfoRight( _F16(0.6) );

Adafruit_ADS1015 ads;
adau17x1 iface;

void processChorus(liteBuf *buf, q31 *lfoData, q31 *interleavedData){
    for(int i=0; i<AUDIO_BUFSIZE; i++){
        uint32_t newOffset = lfoData[i] >> 22;

        while(buf->currentOffset != newOffset){
            if(newOffset > buf->currentOffset){
                //move further back until we reach the correct offset
                if(buf->dptr == buf->data) buf->dptr = buf->data + DELAY_BUFSIZE - 1;
                else buf->dptr--;
                buf->currentOffset++;
            }
            else{
                //move forward until we reach the correct offset
                buf->dptr++;
                if(buf->dptr == buf->data + DELAY_BUFSIZE) buf->dptr = buf->data;
                buf->currentOffset--;
            }
        }

        *buf->head = *interleavedData;
        buf->head++;
        if(buf->head == buf->data + DELAY_BUFSIZE) buf->head = buf->data; //wrap around

        //use the data in dptr
         *interleavedData = *interleavedData + *buf->dptr;
         interleavedData += 2;

         //increment dptr again since we've added to the buffer
         buf->dptr++;
         if(buf->dptr == buf->data + DELAY_BUFSIZE) buf->dptr = buf->data;
    }
}

void audioLoop(q31 *data)
{
    memset(lfoDataL, _F(.999), AUDIO_BUFSIZE*sizeof(q31));
    lfoLeft.getOutput(lfoDataL);
    memset(lfoDataR, _F(.999), AUDIO_BUFSIZE*sizeof(q31));
    lfoRight.getOutput(lfoDataR);

    processChorus(&bufL, lfoDataL, data);
    processChorus(&bufR, lfoDataR, data + 1);
}

void setup(){
    lfoLeft.depth = _F(.25);
    lfoRight.depth = _F(.25);

    ads.setGain(GAIN_TWO);
    ads.begin();

    iface.begin();

    fx.setHook(audioLoop);
    fx.begin();
}

void loop(){
    int16_t adc0, adc1, adc2;

    adc0 = ads.readADC_SingleEnded(0);
    lfoLeft.depth = adc0 * ADC_SCALE_FACTOR/1.5;
    lfoRight.depth = lfoLeft.depth;

    adc1 = ads.readADC_SingleEnded(1);
    lfoLeft.rate = adc1 * RATE_SCALE_FACTOR;

    adc2 = ads.readADC_SingleEnded(2);
    lfoRight.rate = adc2 * RATE_SCALE_FACTOR;
}

