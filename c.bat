@echo off
g++ bass_fx_convolver.cpp -shared -s -O3 -o bass_fx_convolver.dll -L. -lbass -lfftconvolver
g++ test.cpp -L. -lbass