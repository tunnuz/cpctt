GECODE_LIBS = /opt/local

.PHONY: all clean

all: CPCourseTimetabling

CPCourseTimetabling: *.cc *.hh gecode-lns/*.C gecode-lns/*.h Makefile
	g++ -ggdb -std=c++11 -O3 *.cc gecode-lns/*.C -I. -I./gecode-lns -I$(GECODE_LIBS)/include -L$(GECODE_LIBS)/lib -lgecodesearch -lgecodeset -lgecodeint -lgecodekernel -lgecodesupport -lgecodeminimodel -lgecodedriver -lgecodegist -o CPCourseTimetabling

clean:
	rm -rf *.o CPCourseTimetabling
    
