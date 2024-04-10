CC := g++
TAR = bin/FPEPD.exe
CPP = src/*.cpp
Include = -lm -lmpfr -std=c++17 -pthread -I ./include
$(TAR) : $(CPP)
	$(CC) $(CPP) -o $(TAR) $(Include)
.PHONY:
clean:
	rm $(TAR)