CXXFLAGS = -Wall -O3 -std=c++0x -Wno-write-strings -Wno-narrowing
LDFLAGS = -lpthread -lrt -lm -liio -lliquid -lad9361
OBJ = pluto.o pluto_finder.o pluto_driver.o pluto_setup.o pluto_run.o\
kmlib/kmtimer.o kmlib/km_helper.o kmlib/kmfifo.o\
udp/udp.o

default: $(OBJ)
	g++ $(CXXFLAGS) -o pluto $(OBJ) $(LDFLAGS)

clean:
	rm -r *.o
	rm -r kmlib/*.o 
	rm -r udp/*.o 
	
