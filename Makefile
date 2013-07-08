GECODE_LIBS = /opt/local/lib

.PHONY: all clean

all: CPCourseTimetabling

CPCourseTimetabling: *.cc *.hh Makefile
	g++ -std=c++11 -O3 *.cc -I. -L$(GECODE_LIBS) -lgecodesearch -lgecodeset -lgecodeint -lgecodekernel -lgecodesupport -lgecodeminimodel -lgecodedriver -lgecodegist -o CPCourseTimetabling

clean:
	rm -rf *.o CPCourseTimetabling
    
