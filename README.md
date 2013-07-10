# CP+LNS solver for (Curriculum-Based) Course Timetabling

Course Timetabling (CTT) is a popular combinatorial optimization problem which deals with generating university timetables by scheduling weekly lectures, subject to conflicts and availability constraints, while minimizing costs related to resources and user discomfort. 

In [Curriculum-Based Course Timetabling (CB-CTT)](http://satt.diegm.uniud.it/ctt), students enrol to curricula i.e., (possibly overlapping) collections of courses. Lectures of courses pertaining to the same curriculum must, therefore, be scheduled at different times, so that students can attend all courses.

This repository contains the following assets:

* a Constraint Programming (CP) model for the CB-CTT problem written in [GECODE v4](http://www.gecode.org "GECODE"), and
* a Large Neighborhood Search (LNS) solver for the said model (implemented as a GECODE search engine).

**Note** that the LNS engine implemented in this code is a prototype, and does not follow any of the GECODE codebase guidelines. In particular, the whole optimization run is carried out in a single call of `next()`. A more principled LNS engine for GECODE is in the works.

## Usage

Once compiled, the solver can be activated by running:

	$ ./CPCourseTimetabling -time <timeout_in_msec> <cfg_file>
	
Where the `cfg_file` (see `test.cfg` for an example) contains all the necessary data to run the solver, i.e., instance name, LNS parameters, etc., and adheres to the following format:

	instance instances/comp/comp02.ectt
	init-free-variables 1
	max-free-variables 0.05
	ms-per-variable 20
	max-idle-iterations 500
	random-branching random
	random-relaxation 0.15
	
See the [Curriculum-Based Course Timetabling (CB-CTT)](http://satt.diegm.uniud.it/ctt) website for more information on the instance format (and lots of benchmark instances).

## Building

In order to compile the solver you'll need the following prerequisites:

* a C++11-compliant compiler and standard library, and
* GECODE v4 (although the code can be easily ported to GECODE v3 with an upcoming patch).

Also, update the `GECODE_LIBS` variable in the Makefile, so that it points to the correct shared library path, e.g., `/usr/local/lib`. Then, run

	$ make
	
to produce the solver executable.

## Licensing

The code is provided under the MIT License, except for the following files:

* faculty.hh
* faculty.cc

which are needed in order to read the instance files.
**Note** that these files represent an obsolete, but functional, version of the instance reader. The most recent one (though not compatible with the solver's code) is openly available at [http://www.diegm.uniud.it/schaerf/SAS/programmi/TimetablingEL.zip](http://www.diegm.uniud.it/schaerf/SAS/programmi/TimetablingEL.zip).