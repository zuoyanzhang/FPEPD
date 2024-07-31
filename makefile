TAR = bin/geneMPFR.exe
CPP = src/*.cpp
CC := g++
GMP_PREFIX := /opt/homebrew/opt/gmp
MPFR_PREFIX := /opt/homebrew/opt/mpfr
Include = -I$(GMP_PREFIX)/include -I$(MPFR_PREFIX)/include -I./include/ -L$(GMP_PREFIX)/lib -L$(MPFR_PREFIX)/lib -lm -lmpfr
CXXFLAGS = --std=c++17
$(TAR) : $(CPP)
	$(CC) $(CPP) -o $(TAR) $(Include) $(CXXFLAGS)	
.PHONY:
clean:
	rm $(TAR)