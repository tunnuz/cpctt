# CP+LNS solver for (Curriculum-Based) Course Timetabling

Course Timetabling (CTT) is a popular combinatorial optimization problem which deals with generating university timetables by scheduling weekly lectures, subject to conflicts and availability constraints, while minimizing costs related to resources and user discomfort. 

In [Curriculum-Based Course Timetabling (CB-CTT)](http://satt.diegm.uniud.it/ctt), students enrol to curricula i.e., (possibly overlapping) collections of courses. Lectures of courses pertaining to the same curriculum must, therefore, be scheduled at different times, so that students can attend all courses.

This repository contains the following assets:

* a Constraint Programming (CP) model for the CB-CTT problem written in [GECODE v4.2.0](http://www.gecode.org "GECODE"), and
* a Large Neighborhood Search (LNS) solver for the said model (implemented as a GECODE search meta-engine, i.e., an engine having one of the base engine as subcomponent).

**Implementation note** the first version of this solver was based on a less engineered engine, which was not in line with GECODE guidelines. In particular, the whole optimization run used to be carried out in a single call of `next()`. The solver now relies on a more principled LNS meta-engine for GECODE by [Luca Di Gaspero](https://bitbucket.org/ldigaspero) and me, that can be found at [https://bitbucket.org/tunnuz/gecode-lns](https://bitbucket.org/tunnuz/gecode-lns, currently in GECODE 4.1.0 but it can be adjusted to 4.2.0 with simple fixes, see the patched version of gecode-lns from this repository).

## Usage

Once compiled, the solver can be activated by running:

	$ ./CPCourseTimetabling -time <timeout_in_msec> <ctt_instance_file>
	
Where the `ctt_instance_file` is an instance file in CTT or ECTT format (see [http://satt.diegm.uniud.it/ctt](http://satt.diegm.uniud.it/ctt) for more details on the format and a lot of benchmark instances). Additional parameters can be passed to the LNS meta-engine, in particular:

* `-lns_time_per_variable` how much time is dedicated to each variable when the sub-solver is called at each LNS *repair* iteration
* `-lns_constraint_type` the kind of constraining which is done to the solution when the sub-solver is called at each LNS *repair* iteration (strict, loose, …)
* `-lns_max_iterations_per_intensity` maximum number of non-improving iterations before increasing the relaxation intensity
* `-lns_min_intensity` minimum relaxation intensity (the semantics of this value is up to the developer of the model)
* `-lns_max_intensity` maximum relaxation intensity (the semantics of this value is up to the developer of the model)
* `-lns_sa_start_temperature` initial temperature for the Simulated Annealing acceptance criterion
* `-lns_sa_cooling_rate` temperature decay factor for the Simulated Annealing acceptance criterion
* `-lns_sa_cooling_rate` parameter to control *cutoffs*, i.e., number of accepted solutions at each temperature step in the Simulated Annealing acceptance criterion (see [Johnson et al., 1989](http://www-vis.lbl.gov/~aragon/pubs/annealing-pt1.pdf) for more information on cutoffs)

The parameters are set to reasonable defaults.

## Building

In order to compile the solver you'll need the following prerequisites:

* a C++11-compliant compiler and standard library, and
* GECODE v4.2.0

Also, update the `GECODE_LIBS` variable in the Makefile, so that it points to the correct shared library path, e.g., `/usr/local/lib`. Then, run

	$ make
	
to produce the solver executable.

## Licensing

The code is provided under the MIT License, except for the following files:

* faculty.hh
* faculty.cc

which are needed in order to read the instance files.
**Note** that these files represent an obsolete, but functional, version of the instance reader. The most recent one (though not compatible with the solver's code) is openly available at [http://www.diegm.uniud.it/schaerf/SAS/programmi/TimetablingEL.zip](http://www.diegm.uniud.it/schaerf/SAS/programmi/TimetablingEL.zip).