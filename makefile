TAR = bin/geneMPFR.exe
CPP = src/*.cpp
CC := g++
Include = -lm -lmpfr -I ./include/
$(TAR) : $(CPP)
	$(CC) $(CPP) -o $(TAR) $(Include)
.PHONY:
clean:
	rm $(TAR)