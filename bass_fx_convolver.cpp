#include "bass_fx_convolver.h"
#include "bass-addon.h"
#include<windows.h>
#include<cstdio>
#include "FFTConvolver.h"
#include "TwoStageFFTConvolver.h"
using namespace fftconvolver;

#ifndef bassfunc
const BASS_FUNCTIONS *bassfunc=NULL;
#endif

#define MIN_BUFFER_SIZE 64
#define MAX_BUFFER_SIZE 65536

extern ADDON_FUNCTIONS_FX fxfuncs;

inline void cap (DWORD& value, DWORD min, DWORD max) {
if (value<min) value=min;
if (value>max) value=max;
}

struct Convolver {
virtual void process (float* buffer, size_t length) = 0;
virtual int init (BASS_BFX_CONVOLVER& info) = 0;
virtual void reset() = 0;
virtual size_t getChannelCount () = 0;
virtual ~Convolver() = default;
};

struct ConvolverMono: Convolver {
FFTConvolver convolver;
bool initialized;

void process (float* buffer, size_t length) override;
int init (BASS_BFX_CONVOLVER& info) override;
void reset () override { convolver.reset(); }
size_t getChannelCount () override { return 1; }
ConvolverMono (): convolver(), initialized(false) {}
virtual ~ConvolverMono () = default;
};

struct ConvolverMonoTwoStage: Convolver {
TwoStageFFTConvolver convolver;
bool initialized;

void process (float* buffer, size_t length) override;
int init (BASS_BFX_CONVOLVER& info) override;
void reset () override { convolver.reset(); }
size_t getChannelCount () override { return 1; }
ConvolverMonoTwoStage (): convolver(), initialized(false) {}
virtual ~ConvolverMonoTwoStage () = default;
};

template<size_t N>
struct ConvolverMulti: Convolver {
FFTConvolver convolvers[N];
bool initialized;

void process (float* buffer, size_t length) override;
int init (BASS_BFX_CONVOLVER& info) override;
void reset () override { for (size_t i=0; i<N; i++) convolvers[i].reset(); }
size_t getChannelCount () override { return N; }
ConvolverMulti (): convolvers(), initialized(false) {}
virtual ~ConvolverMulti () = default;
};

template<size_t N>
struct ConvolverMultiTwoStage: Convolver {
TwoStageFFTConvolver convolvers[N];
bool initialized;

void process (float* buffer, size_t length) override;
int init (BASS_BFX_CONVOLVER& info) override;
void reset () override { for (size_t i=0; i<N; i++) convolvers[i].reset(); }
size_t getChannelCount () override { return N; }
ConvolverMultiTwoStage (): convolvers(), initialized(false) {}
virtual ~ConvolverMultiTwoStage () = default;
};

template<size_t N>
void deinterleave (float* in, float* out, size_t len) {
for (size_t j=0; j<N; j++)
for (size_t i=0; i<len; i++)
out[j*len+i] = in[i*N+j];
}

template<size_t N>
void interleave (float* in, float* out, size_t len) {
for (size_t j=0; j<N; j++)
for (size_t i=0; i<len; i++)
out[i*N+j] = in[j*len+i];
}

int ConvolverMono::init (BASS_BFX_CONVOLVER& info) {
cap(info.dwShortBufferSize, MIN_BUFFER_SIZE, MAX_BUFFER_SIZE);
return (initialized = convolver.init(info.dwShortBufferSize, info.lpIRBuffer, info.dwIRLength))? 0 : BASS_ERROR_ILLPARAM;
}

int ConvolverMonoTwoStage::init (BASS_BFX_CONVOLVER& info) {
cap(info.dwShortBufferSize, MIN_BUFFER_SIZE, MAX_BUFFER_SIZE);
cap(info.dwLongBufferSize, info.dwShortBufferSize*2, MAX_BUFFER_SIZE);
return (initialized = convolver.init(info.dwShortBufferSize, info.dwLongBufferSize, info.lpIRBuffer, info.dwIRLength))? 0 : BASS_ERROR_ILLPARAM;
}

void ConvolverMono::process (float* in, size_t len) {
if (!initialized) return;
convolver.process(in, in, len);
}

void ConvolverMonoTwoStage::process (float* in, size_t len) {
if (!initialized) return;
float out[len];
convolver.process(in, out, len);
memcpy(in, out, len*sizeof(float));
}

template<size_t N>
int ConvolverMulti<N>::init (BASS_BFX_CONVOLVER& info) {
if (info.nChannels!=1 && info.nChannels!=N) return BASS_ERROR_FORMAT;
cap(info.dwShortBufferSize, MIN_BUFFER_SIZE, MAX_BUFFER_SIZE);
if (info.nChannels==1) {
for (size_t i=0; i<N; i++) {
if (!convolvers[i].init(info.dwShortBufferSize, info.lpIRBuffer, info.dwIRLength)) return BASS_ERROR_ILLPARAM;
}}
else {
auto buf = std::make_unique<float[]>(info.dwIRLength * N);
deinterleave<N>(info.lpIRBuffer, &buf[0], info.dwIRLength);
for (size_t i=0; i<N; i++) {
if (!convolvers[i].init(info.dwShortBufferSize, &buf[i*info.dwIRLength], info.dwIRLength)) return BASS_ERROR_ILLPARAM;
}}
initialized = true;
return 0;
}

template<size_t N>
int ConvolverMultiTwoStage<N>::init (BASS_BFX_CONVOLVER& info) {
if (info.nChannels!=1 && info.nChannels!=N) return BASS_ERROR_FORMAT;
cap(info.dwShortBufferSize, MIN_BUFFER_SIZE, MAX_BUFFER_SIZE);
cap(info.dwLongBufferSize, info.dwShortBufferSize*2, MAX_BUFFER_SIZE);
if (info.nChannels==1) {
for (size_t i=0; i<N; i++) {
if (!convolvers[i].init(info.dwShortBufferSize, info.dwLongBufferSize, info.lpIRBuffer, info.dwIRLength)) return BASS_ERROR_ILLPARAM;
}}
else {
auto buf = std::make_unique<float[]>(info.dwIRLength * N);
deinterleave<N>(info.lpIRBuffer, &buf[0], info.dwIRLength);
for (size_t i=0; i<N; i++) {
if (!convolvers[i].init(info.dwShortBufferSize, info.dwLongBufferSize, &buf[i*info.dwIRLength], info.dwIRLength)) return BASS_ERROR_ILLPARAM;
}}
initialized = true;
return 0;
}

template<size_t N>
void ConvolverMulti<N>::process (float* in, size_t len) {
if (!initialized) return;
size_t pLen = len/N;
float out[len];
deinterleave<N>(in, out, pLen);
for (size_t i=0; i<N; i++) {
float* p = out + i * pLen;
convolvers[i].process(p, p, pLen);
}
interleave<N>(out, in, pLen);
}

template<size_t N>
void ConvolverMultiTwoStage<N>::process (float* in, size_t len) {
if (!initialized) return;
size_t pLen = len/N;
float out1[len], out2[len];
deinterleave<N>(in, out1, pLen);
for (size_t i=0; i<N; i++) {
float *p = out1 + i * pLen, *q = out2 + i * pLen;
convolvers[i].process(p, q, pLen);
}
interleave<N>(out2, in, pLen);
}

void CALLBACK ConvolverProcess (HDSP fx, DWORD ch, void* vBuf, DWORD vLen, void* vInst) {
auto& convolver = *reinterpret_cast<Convolver*>(vInst);
float* buf = reinterpret_cast<float*>(vBuf);
convolver.process(buf, vLen/sizeof(float));
}

void WINAPI ConvolverFree (void* vInst) {
auto convolver = reinterpret_cast<Convolver*>(vInst);
delete convolver;
}

BOOL WINAPI ConvolverReset (void* vInst) {
auto& convolver = *reinterpret_cast<Convolver*>(vInst);
convolver.reset();
return TRUE;
}

BOOL WINAPI ConvolverGetParams (void* vInst, void* vParams) {
auto& convolver = *reinterpret_cast<Convolver*>(vInst);
auto& params = *reinterpret_cast<BASS_BFX_CONVOLVER*>(vParams);
return TRUE;
}

BOOL WINAPI ConvolverSetParams (void* vInst, const void* vParams) {
auto& convolver = *reinterpret_cast<Convolver*>(vInst);
auto& params = *reinterpret_cast<BASS_BFX_CONVOLVER*>(const_cast<void*>(vParams));
std::unique_ptr<float[]> buffer;
if (!params.lpIRBuffer && !params.hIRStream && params.hIRSample) {
params.hIRStream = BASS_SampleGetChannel(params.hIRSample, 0x200002);
}
if (!params.lpIRBuffer && params.hIRStream) {
#define errorf(n) { BASS_StreamFree(params.hIRStream); error(n); }
BASS_CHANNELINFO info;
if (!BASS_ChannelGetInfo(params.hIRStream, &info)) return FALSE;
if (!(info.flags&BASS_STREAM_DECODE)) errorf(BASS_ERROR_DECODE);
if (info.chans!=1 && info.chans!=convolver.getChannelCount()) errorf(BASS_ERROR_FORMAT);
DWORD length = BASS_ChannelGetLength(params.hIRStream, BASS_POS_BYTE);
if (!(info.flags&BASS_SAMPLE_FLOAT)) length*=2;
if (info.flags&BASS_SAMPLE_8BITS) length*=2;
buffer = std::make_unique<float[]>(length/4);
if (!buffer) errorf(BASS_ERROR_MEM);
params.lpIRBuffer = &buffer[0];
params.dwIRLength = length / (info.chans * sizeof(float));
params.nChannels = info.chans;
char* cbuf = reinterpret_cast<char*>(&buffer[0]);
size_t re, pos = 0;
while(pos<length){
re = BASS_ChannelGetData(params.hIRStream, cbuf+pos, std::max(length-pos, 1048576UL) | BASS_DATA_FLOAT);
if (re<=0) break;
pos += re;
}
BASS_StreamFree(params.hIRStream);
#undef errorf
}
if (params.nChannels<=0) params.nChannels = convolver.getChannelCount();
if (!params.lpIRBuffer || params.dwIRLength<=0) error(BASS_ERROR_ILLPARAM);
int result = convolver.init(params);
bassfunc->SetError(result);
return result==0;
}

HFX CALLBACK CreateConvolver (DWORD ch, DWORD type, int priority) {
if (!BASS_GetConfig(BASS_CONFIG_FLOATDSP)) error(BASS_ERROR_NOTAVAIL);
if (!ch) error(BASS_ERROR_HANDLE);
if (type!=BASS_FX_BFX_CONVOLVER && type!=BASS_FX_BFX_CONVOLVER_TWO_STAGE) error(BASS_ERROR_ILLTYPE);
BASS_CHANNELINFO info;
if (!BASS_ChannelGetInfo(ch, &info)) return 0;
Convolver* convolver = nullptr;
switch(type) {
case BASS_FX_BFX_CONVOLVER_TWO_STAGE:
switch (info.chans) {
case 1: convolver = new ConvolverMonoTwoStage(); break;
case 2: convolver = new ConvolverMultiTwoStage<2>(); break;
case 4: convolver = new ConvolverMultiTwoStage<4>(); break;
case 6: convolver = new ConvolverMultiTwoStage<6>(); break;
case 8: convolver = new ConvolverMultiTwoStage<8>(); break;
default: error(BASS_ERROR_FORMAT);
}break;
case BASS_FX_BFX_CONVOLVER:
 switch(info.chans) {
case 1: convolver = new ConvolverMono(); break;
case 2: convolver = new ConvolverMulti<2>(); break;
case 4: convolver = new ConvolverMulti<4>(); break;
case 6: convolver = new ConvolverMulti<6>(); break;
case 8: convolver = new ConvolverMulti<8>(); break;
default: error(BASS_ERROR_FORMAT);
}break;
default: error(BASS_ERROR_ILLTYPE);
}
if (!convolver) error(BASS_ERROR_ILLPARAM);
return bassfunc->SetFX(ch, ConvolverProcess, convolver, priority, &fxfuncs);
}

void InitPlugin () {
GetBassFunc();
bassfunc->RegisterPlugin((void*)CreateConvolver, PLUGIN_FX_ADD);
}

void DeinitPlugin () {
bassfunc->RegisterPlugin((void*)CreateConvolver, PLUGIN_FX_REMOVE);
}

extern "C" BOOL WINAPI __declspec(dllexport)  DllMain (HINSTANCE dll, DWORD reason, LPVOID reserved) {
switch(reason){
case DLL_PROCESS_ATTACH:
InitPlugin();
break;
case DLL_PROCESS_DETACH:
DeinitPlugin();
break;
}
return TRUE;
}

ADDON_FUNCTIONS_FX fxfuncs = {
ConvolverFree,
ConvolverSetParams,
ConvolverGetParams,
ConvolverReset
};
