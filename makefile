ifeq ($(OS),Windows_NT)
EXT_DLL=.dll
else
EXT_DLL=.so
endif

ifeq ($(mode),release)
NAME_SUFFIX=
DEFINES += RELEASE
CXXOPTFLAGS=-s -O3
else
NAME_SUFFIX=d
DEFINES += DEBUG
CXXOPTFLAGS=-g
endif

OBJDIR=obj$(NAME_SUFFIX)/
CXX=g++
CXXFLAGS=-std=gnu++17 -Wextra -shared $(addprefix -D,$(DEFINES)) -mthreads
LDFLAGS=-L. -lbass -lfftconvolver

BASS_FX_CONVOLVER_DLL=bass_fx_convolver$(NAME_SUFFIX)$(EXT_DLL)
BASS_FX_CONVOLVER_SRCS=bass_fx_convolver.cpp
BASS_FX_CONVOLVER_OBJS=$(addprefix $(OBJDIR),$(BASS_FX_CONVOLVER_SRCS:.cpp=.o))

all: $(BASS_FX_CONVOLVER_DLL)
.PHONY: all

clean:
	rm -r $(OBJDIR)

$(BASS_FX_CONVOLVER_DLL): $(BASS_FX_CONVOLVER_OBJS)
	@$(CXX) $(CXXFLAGS) $(CXXOPTFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)%.o: %.cpp $(wildcard %.h)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(CXXOPTFLAGS) -c -o $@ $<
