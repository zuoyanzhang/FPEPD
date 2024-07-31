CC := g++
GMP_PREFIX := /opt/homebrew/opt/gmp
MPFR_PREFIX := /opt/homebrew/opt/mpfr
TAR = bin/FPEPD.exe
CPP = src/*.cpp
Include = -I$(GMP_PREFIX)/include -I$(MPFR_PREFIX)/include -L$(GMP_PREFIX)/lib -L$(MPFR_PREFIX)/lib -lm -lmpfr -std=c++17 -pthread -I ./include
CXXFLAGS = --std=c++17
$(TAR) : $(CPP)
	$(CC) $(CPP) -o $(TAR) $(Include) $(CXXFLAGS)
.PHONY:
clean:
	rm $(TAR)