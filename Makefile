CXXFLAGS = -Wall -O3 -std=c++0x -Wno-write-strings -Wno-narrowing
LDFLAGS = -lpthread -lrt -lm -liio
OBJ = pluto.o pluto_finder.o pluto_setup.o pluto_RX.o pluto_TX.o pluto_driver.o kmtimer.o

default: $(OBJ)
	g++ $(CXXFLAGS) -o pluto $(OBJ) $(LDFLAGS)

clean:
	rm -r *.o
