# Convolution BASS FX plugin

This is a [BASS](http://un4seen.com/) FX plugin allowing to add a convolution effect to any BASS stream.

The effect can be applied to any sream having 1 (mono), 2 (stereo), 4, 6 and 8 audio channels.
Impulse responses must be mono or correspond to the number of audio channels of the stream on which it is applied on.

The convolution engine is based on 
[FFT Convolver by HiFi LowFi](https://github.com/HiFi-LoFi/FFTConvolver)

Short documentation is in bass_fx_convolver.h. Use this file and load the plugin using standard BASS plugin mechanism.

Have fun!
