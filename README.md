# CP+LNS solver for Course Timetabling

Course Timetabling (CTT) is a popular combinatorial optimization problem which deals with generating university timetables by scheduling weekly lectures, subject to conflicts and availability constraints, while minimizing costs related to resources and user discomfort. 

In [Curriculum-Based Course Timetabling (CB-CTT)](http://satt.diegm.uniud.it/ctt), students enrol to curricula i.e., (possibly overlapping) collections of courses. Lectures of courses pertaining to the same curriculum must therefore be scheduled at different times, so that students can attend all courses.

This repository contains the following assets:

* a Constraint Programming (CP) model for the CB-CTT problem written in [GECODE v4](http://www.gecode.org "GECODE"), and
* a Large Neighborhood Search (LNS) solver for the said model.

## Usage

Once compiled, the solver can be activated by running

	$ ./CPCourseTimetabling -time <timeout_in_msec> <cfg_file>
	
Where the `cfg_file` (see `test.cfg` for an example) contains all the necessary data to run the solver, i.e., instance name, LNS parameters, etc., and adheres to the following format:

	instance instances/comp/comp02.ectt
	init-free-variables 1
	max-free-variables 0.05
	ms-per-variable 20
	max-idle-iterations 500
	random-branching random
	random-relaxation 0.15
	
See the [Curriculum-Based Course Timetabling (CB-CTT)](http://satt.diegm.uniud.it/ctt) website for more information on the instance format.

## Building

In order to compile you'll need the following prerequisites:

* a C++11 compliant version of GCC,
* GECODE v4 (although the code can be easily ported to GECODE v3) 

update the `GECODE_LIBS` variable in the Makefile, then running

	$ make
	
will produce an executable called CPCourseTimetabling, which is the solver itself.

## Licensing

The code is provided under the MIT License, except for the following files:

* faculty.hh
* faculty.cc

which are needed to read the instance files. **Note** these files are an old version of the instance reader, a new one (though not compatible with the code in this repository) is openly available at [http://www.diegm.uniud.it/schaerf/SAS/programmi/TimetablingEL.zip](http://www.diegm.uniud.it/schaerf/SAS/programmi/TimetablingEL.zip).