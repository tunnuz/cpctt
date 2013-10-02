GECODE_LIBS = /opt/local

.PHONY: all clean

all: CPCourseTimetabling

CPCourseTimetabling: *.cc *.hh Makefile
	g++ -std=c++11 -O3 *.cc -I. -I./gecode-lns -I$(GECODE_LIBS)/include -L$(GECODE_LIBS)/lib -lgecodesearch -lgecodeset -lgecodeint -lgecodekernel -lgecodesupport -lgecodeminimodel -lgecodedriver -lgecodegist -o CPCourseTimetabling

clean:
	rm -rf *.o CPCourseTimetabling
    
