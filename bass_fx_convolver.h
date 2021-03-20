/**
Convolution BASS FX plugin
Based on FFT Convolver https://github.com/HiFi-LoFi/FFTConvolver
*/
#ifndef ___BASS_FX_CONVOLVER___
#define ___BASS_FX_CONVOLVER___
#include "bass.h"

#define BASS_FX_BFX_CONVOLVER 0x10FFD
#define BASS_FX_BFX_CONVOLVER_TWO_STAGE 0x10FFE

struct BASS_BFX_CONVOLVER {
DWORD dwShortBufferSize; // buffer size in samples
DWORD dwLongBufferSize; // buffer size in samples
float* lpIRBuffer; // Impulse response buffer
DWORD dwIRLength; // Impulse response buffer length in samples
DWORD nChannels; // number of channels of the IR buffer (0=auto, 1=mono, 2=stereo, ...)
HSTREAM hIRStream; // Impulse response stream, only used if lpIRBuffer==NULL; stream must be a decoding channel, automatically free after BASS_FXSetParameters has been called.
HSAMPLE hIRSample; // Impulse response  HSAMPLE, only used if lpIRBuffer==NULL && hIRStream==NULL. Require BASS_STREAMCHAN_SAMPLE flag new in BASS 2.4.15.
};

#endif
