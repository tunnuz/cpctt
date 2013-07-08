#ifndef CP_CTT_CBCTT_hh
#define CP_CTT_CBCTT_hh

#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <gecode/search.hh>
#include <gecode/driver.hh>
#include <gecode/gist.hh>
#include "faculty.hh"
#include "LNSSpace.hh"
#include "LNS.hh"
#include <queue>
#include <cmath>
#include <map>

#undef HARD_ROOM_CAPACITY
#define PRINT_JSON
#undef PRINT_JSON
#undef HARD_MINIMUM_WORKING_DAYS
#undef HARD_CURRICULUM_COMPACTNESS
#undef HARD_ROOM_STABILITY

#define HARD_CONFLICTS 1                // conflicting lectures are forbidden by distinct constraint
#undef HARD_CONFLICTS

#define HARD_DUPLICATES 1               // overlappings are forbidden by distinct constraint
#undef HARD_DUPLICATES

#define pass

#define ROOM_CAPACITY_COST 1            // per standing student
#define MINIMUM_WORKING_DAYS_COST 5     // per day below minimum
#define CURRICULUM_COMPACTNESS_COST 2   // per non-adjacent lectures in same day
#define ROOM_STABILITY_COST 1           // per extra room used for the lectures of a course

using namespace Gecode;
using namespace std;

/** CP model for the Course-Based Curriculum Time Tabling Problem */
class CBCTT : public DeferredBranchingSpace<MinimizeScript>
{

public:

    /** Input (instance information) */
    static Faculty in;

    /** Course conflicts */
    IntVarArray course_conflicts;
    
    /** Conflicting lectures. */
    IntVarArray conflicting_lectures;

    /** Conflicts (total) */
    IntVar conflicts;
    
    /** Period of each lecture. */
    IntVarArray period;

    /** Room of each lecture. */
    IntVarArray room;

    /** Day of each lecture */
    IntVarArray day;

    /** For each lecture, which room slot (room, period) is in */
    IntVarArray roomslot;
    
    /** Duplicate roomslots */
    IntVar duplicates;

    /** Room capacity cost component */
    IntVar room_capacity_cost;
    
    /** Lectures which cause room capacity cost */
    IntVarArray room_capacity_deviation;

    /** Room stability cost component */
    IntVar room_stability_cost;
    
    /** Costs caused by room stability */
    IntVarArray room_stability_deviation;

    /** Minimum working days cost component */
    IntVar minimum_working_days_cost;
    
    /** Lectures that cause minimum working days cost */
    IntVarArray minimum_working_days_deviation;

    /** Curriculum compactness cost component */
    IntVar curriculum_compactness_cost;

    /** Lectures that cause curriculum compactness costs */
    IntVarArray curriculum_compactness_deviation;
    
    BoolVarArray lecture_compactness;
    
    /** Solution cost */
    IntVar z;
    
    bool debug;
    
protected:
    
    vector<unsigned int> index_of_start_lecture;

public:

    /** Constructor. 
     *  @param o instance options (e.g. instance name)
     */
    CBCTT(const InstanceOptions& o) : debug(o.model() == 0)
    {

	    CBCTT::in = Faculty(o.instance());
        
        /*************************************
         * PARAMETERS                        *
         ************************************/

        // Load instance in the model
        unsigned int total_lectures = 0;

        // Total number of roomslots is periods times rooms
        unsigned int total_roomslots = in.Periods() * in.Rooms();

        // For each course, keep the index of the starting lecture
        index_of_start_lecture.resize(in.Courses());
        for (unsigned int c = 0; c < in.Courses(); c++)
        {
            index_of_start_lecture[c] = total_lectures;
            total_lectures += in.CourseVector(c).Lectures();
        }

        // For each lecture, keep the index of the course
        map<unsigned int, unsigned int> course_of_lecture;
        for (unsigned int c = 0, l = 0; c < in.Courses(); c++)
            for (unsigned int i = 0; i < in.CourseVector(c).Lectures(); i++)
                course_of_lecture[l++] = c;

        /************************************
        * VARIABLES                         *
        ************************************/

        // Decision variable 
        roomslot = IntVarArray(*this, total_lectures, 0, total_roomslots - 1);
        duplicates = IntVar(*this, 0, total_lectures);
        
        // Auxiliary variables (to facilitate posting of constraints)
        period = IntVarArray(*this, total_lectures, 0, in.Periods() - 1);
        
        
        IntVarArgs timeslot(*this, total_lectures, 0, in.PeriodsPerDay() - 1);
        day = IntVarArray(*this, total_lectures, 0, in.Days() - 1);
        room  = IntVarArray(*this, total_lectures, 0, in.Rooms() - 1);
        
        // Relations between auxiliary variables and main decision variable
        for (unsigned int l = 0; l < total_lectures; l++)
        {
            rel(*this, period[l] == roomslot[l] / in.Rooms());
            rel(*this, timeslot[l] == period[l] % in.PeriodsPerDay());
            rel(*this, day[l] == period[l] / in.PeriodsPerDay());
            rel(*this, room[l] == roomslot[l] % in.Rooms());
        }
        
        //for (unsigned int r = 0; r < total_roomslots; r++)
        //    for(unsigned int l = 0; l < total_lectures; l++)
        //        rel(*this, (roomslot[l] == r) >> (lecture[r] == l));

        /************************************
        * CONSTRAINTS                       *
        ************************************/

        // [Lectures]: all lectures must be scheduled, implied by size of roomslots and next constraint
        pass; 

        // [RoomOccupancy] lectures must be scheduled different roomslots
#ifdef HARD_DUPLICATES
        distinct(*this, roomslot);
#else
        // Number of duplicates
        nvalues(*this, roomslot, IRT_EQ, duplicates);
#endif
        
        // [Conflicts] lectures of (1) same course, (2) course in the same curriculum or (3) taught by the same professor must be scheduled different periods

        // 1. Lectures of same course must be scheduled different periods
        for (unsigned int c = 0; c < in.Courses(); c++)
            for (unsigned int l1 = 0; l1 < in.CourseVector(c).Lectures() - 1; l1++)
                for (unsigned int l2 = l1 + 1; l2 < in.CourseVector(c).Lectures(); l2++)
                {
                    int cl1 = index_of_start_lecture[c]+l1, cl2 = index_of_start_lecture[c]+l2;

                    // We use < instead of == for symmetry breaking
                    rel(*this, period[cl1] < period[cl2]);
                    
                    // We enforce the constraint on roomslot, even if this is implied  (redundant)
                    rel(*this, roomslot[cl1] < roomslot[cl2]);

                    // Implies the same on days and timeslots  (redundant)
                    rel(*this, (day[cl1] == day[cl2]) >> (timeslot[cl1] < timeslot[cl2]));
                    rel(*this, (timeslot[cl1] == timeslot[cl2]) >> (day[cl1] < day[cl2]));
                    
                }

        // 2,3. Lectures in courses by the same teacher, or courses in the same curriculum must be scheduled different periods 

#ifndef HARD_CONFLICTS
        int cc = ((in.Courses()-1)*(in.Courses()))/2;
        IntVarArgs course_conflicts(*this, cc, 0, total_lectures);
        IntVarArgs course_period_cardinality(*this, cc, 0, in.Periods());
        
        cc = 0;
#endif
        for (unsigned int c1 = 0; c1 < in.Courses() - 1; c1++)
        {
            for (unsigned int c2 = c1 + 1; c2 < in.Courses(); c2++)
            {                
                // If courses are not in conflict, ignore all their lectures
                if (!in.Conflict(c1, c2))
                {
                
#ifndef HARD_CONFLICTS
                    rel(*this, course_conflicts[cc] == 0);
                    cc++;
#endif
                    continue;
                }
                
                // Make up array of variables relative to the period of the specified lectures
                IntVarArgs period_of_incompatible_lectures =
                    period.slice(index_of_start_lecture[c1], 1, in.CourseVector(c1).Lectures()) +
                    period.slice(index_of_start_lecture[c2], 1, in.CourseVector(c2).Lectures());
                
#ifndef HARD_CONFLICTS
                
                nvalues(*this, period_of_incompatible_lectures, IRT_EQ, course_period_cardinality[cc]);
                course_conflicts[cc] = expr(*this, period_of_incompatible_lectures.size() - course_period_cardinality[cc]);
                
                // Use set to get unique periods (alternate formulation)
                /*
                SetVar s_p(*this, IntSet::empty, IntSet(0, in.Periods()-1));
                rel(*this, SOT_UNION, period_of_incompatible_lectures, s_p);
                cardinality(*this, s_p, course_period_cardinality[cc]);
                
                rel(*this, course_conflicts[cc] == (period_of_incompatible_lectures.size() - course_period_cardinality[cc]));
                */
                
                cc++;

#else
                // State that all these periods must be different
                distinct(*this, period_of_incompatible_lectures);
#endif
            }
        }

#ifndef HARD_CONFLICTS
        // The sum of conflicts if a component of the cost function
        conflicts = expr(*this, sum(course_conflicts));
#else
        conflicts = expr(*this, 0);
#endif
        
        
        // [LNS: ConflictingLectures] auxiliary variable to facilitate LNS relaxation
        conflicting_lectures = IntVarArray(*this, total_lectures, 0, total_lectures);
        
        for (int l1 = 0; l1 < total_lectures; l1++)
        {
            int c1 = in.LectureCourse(l1);
            
            IntVarArgs candidate_conflicts;
            
            for (int c2 = 0; c2 < in.Courses(); c2++)
            {
                if (c2 == c1 || !in.Conflict(c1,c2))
                    continue;
                
                // Otherwise
                candidate_conflicts << period.slice(index_of_start_lecture[c2], 1, in.CourseVector(c2).Lectures());
            }
            
            /*
            SetVar s(*this, IntSet::empty, IntSet(0, total_lectures));
            rel(*this, SOT_UNION, candidate_conflicts, conflicting_lectures[l1]);
            
            IntVar c;
            */
            count(*this, candidate_conflicts, period[l1], IRT_EQ, conflicting_lectures[l1]);
        }
        
        
        // [Availabilities] Some courses may not be available in some periods, scheduling of related lectures must be handled accordingly
        for (unsigned int c = 0; c < in.Courses(); c++)
            for (unsigned int p = 0; p < in.Periods(); p++)
            {
                // Skip constraint posting if course is available
                if (in.Available(c, p))
                    continue;

                // Post constraint on each lecture of the course 
                for (unsigned int l = 0; l < in.CourseVector(c).Lectures(); l++)
                {
                    rel(*this, period[index_of_start_lecture[c] + l] != p);

                    // As before, this is implied, but nonetheless it helps propagation (redundant)
                    for (unsigned int r = 0; r < in.Rooms(); r++)
                        rel(*this, roomslot[index_of_start_lecture[c] + l] != p * in.Rooms() + r);
                }
            }

#ifdef HARD_ROOM_CAPACITY

        // [RoomCapacity] (Hard) lectures must be scheduled in rooms compatible with their number of students  
        for (unsigned int c = 0; c < in.Courses(); c++)
            for (unsigned int r = 0; r < in.Rooms(); r++)
            {
                // If number of students in course is compatible, skip constraint posting
                if (in.CourseVector(c).Students() <= in.RoomVector(r).Capacity())
                    continue;

                // Forbid each lecture of this course to be scheduled in room r 
                for (unsigned int l = 0; l < in.CourseVector(i).Lectures(); l++)
                {
                    rel(*this, room[index_of_start_lecture[c] + l] != r);

                    // As before, this is implied, but nonetheless it helps propagation (redundant)
                    for (unsigned int p = 0; p < in.Periods(); p++)
                        rel(*this, roomslot[index_of_start_lecture[c] + l] != p * in.Rooms() + r);
                }
            }

        // RoomCapacity component doesn't influence the cost function
        room_capacity_cost = expr(*this, 0);

#else
        // [RoomCapacity] (Soft) lectures should be scheduled in rooms compatible with their number of students
        IntArgs room_capacity(in.Rooms());
        for (unsigned int r = 0; r < in.Rooms(); r++)
            room_capacity[r] = in.RoomVector(r + 1).Capacity();

        room_capacity_deviation = IntVarArray(*this, total_lectures);
        for (unsigned int l = 0; l < total_lectures; l++)
        {
            IntVar room_l_occupation = expr(*this, element(room_capacity, room[l]));
            room_capacity_deviation[l] = expr(*this, max(0, in.CourseVector(course_of_lecture[l]).Students() - room_l_occupation));
        }

        room_capacity_cost = expr(*this, sum(room_capacity_deviation));
#endif

        // [RoomStability] (Soft) all lectures of a course should be given in the same room 

        room_stability_deviation = IntVarArray(*this, in.Courses(), 0, total_lectures);
        for (unsigned int c = 0; c < in.Courses(); c++)
            // Count the number of different rooms used by this course
            nvalues(*this, room.slice(index_of_start_lecture[c], 1, in.CourseVector(c).Lectures()), IRT_EQ, room_stability_deviation[c]);

        // Take the sum of all different rooms by course, subtract one room per course
        room_stability_cost = expr(*this, sum(room_stability_deviation) - in.Courses());

        // [MinimumWorkingDays] lectures of each course must be scheduled in at least a given number of working days
        minimum_working_days_deviation = IntVarArray(*this, in.Courses(), 0, total_lectures);
        IntVarArgs days(*this, in.Courses(), 0, total_lectures);
        
        for(unsigned int c = 0; c < in.Courses(); c++)
        {
            // Note to self: there is no singular IntArg, to avoid posting IntVar use plural IntVarArg (days)
            
            nvalues(*this, day.slice(index_of_start_lecture[c],1, in.CourseVector(c).Lectures()), IRT_EQ, days[c]);
            minimum_working_days_deviation[c] = expr(*this, max(0, in.CourseVector(c).MinWorkingDays() - days[c]));
            
            // Use set to compute unique days (alternate formulation)
            /*
            SetVar s_d(*this, IntSet::empty, IntSet(0, in.Days()-1));
            rel(*this, SOT_UNION, day.slice(index_of_start_lecture[c],1, in.CourseVector(c).Lectures()), s_d);
            cardinality(*this, s_d, days[c]);
            minimum_working_days_deviation[c] = expr(*this, max(0, in.CourseVector(c).MinWorkingDays() - days[c]));
            */
        }

        minimum_working_days_cost = expr(*this, sum(minimum_working_days_deviation));

        // [CurriculumCompactness] (Soft) all lectures of a curriculum should be adjacent to each other within the same day 

        curriculum_compactness_deviation = IntVarArray(*this, in.Curricula(), 0, total_lectures);
        lecture_compactness = BoolVarArray(*this, in.TotalLectures(), 0, 1);

        for(unsigned int q = 0; q < in.Curricula(); q++)
        {                               
            // Gather all lectures in the same curriculum
            vector<int> lectures;
            for (unsigned int c : in.CurriculaVector(q).members)
                for (unsigned int l = 0; l < in.CourseVector(c).Lectures(); l++)
                    lectures.push_back(index_of_start_lecture[c]+l);

            // Collect periods of lectures of this curriculum
            IntVarArgs q_periods;
            SetVar sq_periods(*this, IntSet::empty, IntSet(0, in.Periods()-1));
            for(unsigned int li = 0; li < lectures.size(); li++)
                q_periods << period[lectures[li]];

            // "Channel" IntVarArray and SetVar
            rel(*this, SOT_UNION, q_periods, sq_periods);   

            // List of violations for this curriculum
            BoolVarArgs violations(*this, (int)lectures.size(), 0, 1);
            unsigned int li = 0;

            for(unsigned int lix = 0; lix < lectures.size(); lix++)
            {
                unsigned int l = lectures[lix];

                // Period before
                SetVar before(*this, IntSet::empty, IntSet(0, in.Periods()-1), 0, 1);
                rel(*this, (timeslot[l] == 0) >> (before == IntSet::empty)); 
                rel(*this, (timeslot[l] != 0) >> (before == singleton(period[l] - 1))); 

                // Period after
                SetVar after(*this, IntSet::empty, IntSet(0, in.Periods()-1), 0, 1);    
                rel(*this, (timeslot[l] == in.PeriodsPerDay() - 1) >> (after == IntSet::empty)); 
                rel(*this, (timeslot[l] != in.PeriodsPerDay() - 1) >> (after == singleton(period[l] + 1)));            

                // Intersection between periods before/after and periods of the same curriculum
                SetVar adjacent = expr(*this, (before | after) & sq_periods);

                // Count cardinality of intersection
                IntVar c(*this, 0, 2);                          
                cardinality(*this, adjacent, c);

                // If cardinality is zero, we have a violation
                violations[li] = expr(*this, (c == 0));
                lecture_compactness[l] = expr(*this, (c == 0));
                
                li++;
            }

            // Accumulate curriculum violations
            curriculum_compactness_deviation[q] = expr(*this, sum(violations));
        }

        // Accumulate all violations
        curriculum_compactness_cost = expr(*this, sum(curriculum_compactness_deviation));

        // Cost function
        z = expr(*this, 
            room_capacity_cost * ROOM_CAPACITY_COST +
            room_stability_cost * ROOM_STABILITY_COST +
            minimum_working_days_cost * MINIMUM_WORKING_DAYS_COST +
            curriculum_compactness_cost * CURRICULUM_COMPACTNESS_COST
        );
        
        rel(*this, z >= 0);

    }

    virtual void post_best_branching();

    virtual void post_random_branching();
    
    CBCTT(bool share, CBCTT& s) : DeferredBranchingSpace<MinimizeScript>(share, s), debug(s.debug)
    {
        // Decision var
        roomslot.update(*this, share, s.roomslot);
        index_of_start_lecture = s.index_of_start_lecture;

        // Cost vars
        room_capacity_cost.update(*this, share, s.room_capacity_cost);
        room_stability_cost.update(*this, share, s.room_stability_cost);
        minimum_working_days_cost.update(*this, share, s.minimum_working_days_cost);
        curriculum_compactness_cost.update(*this, share, s.curriculum_compactness_cost);
        conflicts.update(*this, share, s.conflicts);
        duplicates.update(*this, share, s.duplicates);
        course_conflicts.update(*this, share, s.course_conflicts);
        conflicting_lectures.update(*this, share, s.conflicting_lectures);
        period.update(*this, share, s.period);
        room.update(*this, share, s.room);
        day.update(*this, share, s.day);
        
        // Deviations
        room_capacity_deviation.update(*this, share, s.room_capacity_deviation);
        room_stability_deviation.update(*this, share, s.room_stability_deviation);
        minimum_working_days_deviation.update(*this, share, s.minimum_working_days_deviation);
        curriculum_compactness_deviation.update(*this, share, s.curriculum_compactness_deviation);
        lecture_compactness.update(*this, share, s.lecture_compactness);
        
        // Cost
        z.update(*this, share, s.z);
    }

    virtual Space* copy(bool share)
    {
        return new CBCTT(share, *this);
    }

    virtual IntVar cost() const
    {
        return z;
    }

    virtual void print(ostream& os = cout) const
    {
#ifdef PRINT_JSON
        os << "{ \"duplicates\": "<< (in.TotalLectures() - duplicates.val()) <<", \"conflicts\": " << conflicts.val() << ", \"cost\": " << z.val() << ", \"room_capacity_cost\": " << room_capacity_cost.val() << ", \"room_stability_cost\": " << room_stability_cost.val() << ", \"min_working_days_cost\": " << minimum_working_days_cost.val() << ", \"curriculum_compactness_cost\": " << curriculum_compactness_cost.val() << " }" << endl;
        
        if (!debug)
            exit(0);
        
        return;
#endif
        
        
        
        // Printing current search state
        os << "Lecture's roomslot: " << roomslot << endl;
        //os << "Conflicting lectures: " << conflicting_lectures << endl;
        os << "-----------------------" << endl;
        os << "Room capacity\t" << room_capacity_cost << " (x" << ROOM_CAPACITY_COST << ")" << endl;
        os << "Room stability\t" << room_stability_cost<< " (x" << ROOM_STABILITY_COST << ")" << endl;
        os << "Min w. days\t" << minimum_working_days_cost<< " (x" << MINIMUM_WORKING_DAYS_COST << ")" << endl;
        os << "Curr. compact.\t" << curriculum_compactness_cost<< " (x" << CURRICULUM_COMPACTNESS_COST << ")" << endl;
        os << "Conflicts\t" << conflicts << endl;
        //os << "Compactness\t" << lecture_compactness << endl;
        os << "-----------------------" << endl;
        os << "Tot.\t\t" << z << endl;

        if (!roomslot.assigned())
            return;

        // Post solution
        Timetable t(in);
        map<int, int> course_of_lecture;
        int k = 0;
        for (int i = 0; i < in.Courses(); i++)
        {
            for (int j = 0; j < in.CourseVector(i).Lectures(); j++)
                course_of_lecture[k++] = i;
        }
        for (int i = 0; i < roomslot.size(); i++)
        {
            int room = roomslot[i].val() % in.Rooms();
            int period = roomslot[i].val() / in.Rooms();
            t(course_of_lecture[i], period) = room + 1;
        }
    }
    
    /** Prevent assignment of the same period to courses of the same curriculum or teached by the same teacher */
    void post_hard_conflicts()
    {
        for (unsigned int c1 = 0; c1 < in.Courses() - 1; c1++)
        {
            for (unsigned int c2 = c1 + 1; c2 < in.Courses(); c2++)
            {
                // If courses are not in conflict, ignore all their lectures
                if (!in.Conflict(c1, c2))
                    continue;
                
                // Make up array of variables relative to the period of the specified lectures
                IntVarArgs period_of_incompatible_lectures =
                period.slice(index_of_start_lecture[c1], 1, in.CourseVector(c1).Lectures()) +
                period.slice(index_of_start_lecture[c2], 1, in.CourseVector(c2).Lectures());
               
                // State that all these periods must be different
                distinct(*this, period_of_incompatible_lectures);
            }
        }
    }
    
    /** Prevent assignment of the same roomslot to two lectures */
    void post_hard_duplicates()
    {
        distinct(*this, roomslot);
        duplicates = expr(*this, in.TotalLectures()); // necessarily
    }

    /** Prevent a lecture from producing a conflict. */
    void post_hard_conflicts(unsigned int lecture)
    {
        rel(*this, conflicting_lectures[lecture] == 0);
        
        int lecture_course = in.LectureCourse(lecture);
        
        distinct(*this, period.slice(index_of_start_lecture[lecture_course], 1, in.CourseVector(lecture_course).Lectures()));
        
        for (unsigned int c = 0; c < in.Courses(); c++)
        {
            if (c == lecture_course)
                continue;
            
            if (!in.Conflict(lecture_course,c))
                continue;
            
            for (unsigned int l = 0; l < in.CourseVector(c).Lectures(); l++)
                rel(*this, period[lecture] != period[index_of_start_lecture[c]+l]);
            
        }
    }

    void load(const char* s)
    {
        cerr << "Checking " << s << endl;
        cerr << endl;

        // Open solution file
        ifstream sf;
        sf.open(s);

        // Build timetable
        Timetable t(in);
        sf >> t;


        unsigned int periods = in.Periods();
        unsigned int courses = in.Courses();
        unsigned int rooms = in.Rooms();

        int k = 0;
        for (unsigned int c = 0; c < courses; c++)
        {
            for (unsigned int l = 0; l < in.CourseVector(c).Lectures(); l++)
            {
                int skipped = 0;
                for (unsigned int p = 0; p < periods; p++)
                {
                    if (t(c, p) != 0 && skipped == l)
                    {
                        rel(*this, roomslot[k] == (p*rooms)+(t(c,p)-1));
                        cerr << "Lecture " << k << " in roomslot " << ((p*rooms)+(t(c,p)-1)) << endl;
                        break;
                    }
                    else if (t(c,p) != 0)
                        skipped++;
                }


                k++;
            }
        }

        SpaceStatus status = this->status();
        cerr << endl;
        switch(status)
        {
            case Gecode::SS_SOLVED:
                cerr << "SS_SOLVED" << endl;
                break;
            case Gecode::SS_BRANCH:
                cerr << "SS_BRANCH" << endl;
                break;
            default:
                cerr << "SS_FAILED" << endl;
                break;
        }

        print(cerr);

    }

    virtual void compare(const Space& s, ostream& os = cout) const
    {
        os << Gist::Comparator::compare<IntVar>("Roomslots", roomslot, static_cast<const CBCTT&>(s).roomslot);
    }
};

class LNSCBCTT : public LNSSpace<CBCTT>
{
public:


    LNSCBCTT(const InstanceOptions& o) : LNSSpace<CBCTT>(o) { }

    LNSCBCTT(bool share, LNSCBCTT& t) : LNSSpace<CBCTT>(share, t) { }
    
    unsigned int relaxable_vars()
    {
        return roomslot.size();
    }
    
    /** 
    Relax variables according to heuristics based on the the constraints
    that are yet to satisfy or the cost components that are yet to minimize. 
    For each heuristic, the number of actually freed variables is recorded 
    (freed), in the end, (free+freed random variables are freed).
     */
    unsigned int relax(LNSSpace<CBCTT>* tentative, unsigned int free)
    {
        typedef Gecode::Search::Sequential::LNS<LNSCBCTT> MyLNS;
        
        // Counter to keep track of really freed variables
        int freed = 0;
        
//#ifdef TOTALLY_RANDOM
        double rn = (double)rand() / (double)RAND_MAX;
        MyLNS::random = rn < MyLNS::random_relaxation;
//#endif

        // Partition slots in conflicting, non conflicting and all
        vector<int> all;
        vector<int> conflicting;
        
        for (int i = 0; i < roomslot.size(); i++)
        {
            all.push_back(i);
            if (conflicting_lectures[i].val() > 0)
                conflicting.push_back(i);
        }


        if (violations())
        {
            if (conflicts.val())
            {
                // Pick random conflicting lecture
                random_shuffle(conflicting.begin(), conflicting.end());
                
                // Budget of conflicts (1)
                int conf = 1;
                
                while (conf > 0 && !conflicting.empty())
                {
                    int to_fix = conflicting.back();

                    // Relax it
                    if (find(all.begin(), all.end(), to_fix) != all.end())
                    {
                        all.erase(find(all.begin(), all.end(), to_fix));
                        freed++;
                    
                        // Enforce resolution of this conflict
                        tentative->post_hard_conflicts(to_fix);
                        
                        // Relax all conflicting lectures related to to_fix (CAN FREE MORE THAN 'FREE' VARIABLES)
                        for (int l = 0; l < in.TotalLectures(); l++)
                        {
                            if (to_fix == l)
                                continue;
                            
                            // Relax conflicting lectures in same course
                            if (in.LectureCourse(l) == in.LectureCourse(to_fix))
                                if (period[l].assigned() && period[to_fix].val() == period[l].val())
                                    if (find(all.begin(), all.end(), l) != all.end())
                                    {
                                        all.erase(find(all.begin(), all.end(), l));
                                        // tentative->post_hard_conflicts(l);
                                        freed++;
                                    }
                            
                            // Relax conflicting lectures from other courses
                            if (in.Conflict(in.LectureCourse(to_fix), in.LectureCourse(l)))
                                if (period[l].assigned() && period[to_fix].val() == period[l].val())
                                    if (find(all.begin(), all.end(), l) != all.end())
                                    {
                                        all.erase(find(all.begin(), all.end(), l));
                                        // tentative->post_hard_conflicts(l);
                                        freed++;
                                    }
                        }
                        
                        conflicting.pop_back();
                    }
                    else
                    {
                        conflicting.pop_back();
                        continue;
                    }
                    
                    conf--;
                }
            }
            
            // Try to solve duplicates
            else if (duplicates.val() < in.TotalLectures())
            {
                /*
                vector<unsigned int> occ(in.TotalLectures(), 0);
                vector<unsigned int> to_fix;
                
                for (unsigned int l = 0; l < in.TotalLectures(); l++)
                {
                    occ[roomslot[l].val()]++;
                    if (occ[roomslot[l].val()] > 1 && find(all.begin(), all.end(), l) != all.end())
                        to_fix.push_back(l);
                }
                
                random_shuffle(to_fix.begin(), to_fix.end());
                
                // Release a number of variables to fix
                unsigned int dupfix = free;
                while (dupfix > 0 && !to_fix.empty())
                {
                    if (find(all.begin(), all.end(), to_fix.back()) != all.end())
                    {
                        all.erase(find(all.begin(), all.end(), to_fix.back()));
                        to_fix.pop_back();
                        freed++;
                        dupfix--;
                    }
                    else
                    {
                        to_fix.pop_back();
                        continue;
                    }
                }*/
            }
        }
        else
        {
            // Sometimes act randomly
            if (MyLNS::random)
            {
                vector<int> all;
                for (int i = 0; i < roomslot.size(); i++)
                    all.push_back(i);
                
                random_shuffle(all.begin(), all.end());
                
                for (int i = 0; i < free; i++)
                    all.pop_back();
                
                
                for (int s : all)
                    rel(*tentative, tentative->roomslot[s] == roomslot[s].val());
                
                return free;
            }
            
            bool distribute = false;
            
            if (distribute)
            {
                // Compute percentage of variables freed for each cost component
                float all_cost = room_capacity_cost.val() + room_stability_cost.val() + minimum_working_days_cost.val() + curriculum_compactness_cost.val();
                
                cerr << "Total freeable: " << free;
                
                // Partition relaxable variables: room capacity
                float room_capacity = ceil(((float) room_capacity_cost.val() / all_cost) * free);
                
                cerr << " room capacity: " << room_capacity;
                
                for(unsigned int l = 0; l < in.TotalLectures() && room_capacity >= 1; l++)
                    if (room_capacity_deviation[l].val() > 0 && find(all.begin(), all.end(), l) != all.end())
                    {
                        all.erase(find(all.begin(), all.end(), l));
                        freed++;
                        room_capacity--;
                    }
                
                // Partition relaxable variables: room stability
                float room_stability = ceil(((float) room_stability_cost.val() / all_cost) * free);
                
                cerr << " room stab.: " << room_stability;
                
                vector<unsigned int> unstable_courses;
                for(unsigned int c = 0; c < in.Courses(); c++)
                    if (room_stability_deviation[c].val() > 0)
                        unstable_courses.push_back(c);
                
                random_shuffle(unstable_courses.begin(), unstable_courses.end());
                
                for (unsigned int ci  = 0; ci < unstable_courses.size() && room_stability >= 1; ci++)
                {
                    unsigned int c = unstable_courses[ci];
                    
                    // Check for room stability variables budget is performed outside to avoid splitting courses
                    for(int l = 0; l < in.CourseVector(c).Lectures();l++)
                    {
                        if (find(all.begin(), all.end(), index_of_start_lecture[c]+l) != all.end())
                        {
                            all.erase(find(all.begin(), all.end(), index_of_start_lecture[c]+l));
                            freed++;
                            room_stability--;
                        }
                    }
                }
                
                // Partition relaxable variables: working days (alert: works on courses)
                float working_days = ceil(((float) minimum_working_days_cost.val() / all_cost) * free);
                
                cerr << " working days: " << working_days;
                
                for(unsigned int c = 0; c < in.Courses() && working_days >= 1; c++)
                    if (minimum_working_days_deviation[c].val() > 0)
                    {
                        vector<int> lectures;
                        for (int l = 0; l < in.CourseVector(c).Lectures(); l++)
                            lectures.push_back(index_of_start_lecture[c]+l);
                        
                        random_shuffle(lectures.begin(), lectures.end());
                        
                        for (int l : lectures)
                        {
                            if (find(all.begin(), all.end(), l) != all.end())
                            {
                                if (working_days < 1)
                                    break;
                                
                                all.erase(find(all.begin(), all.end(), l));
                                freed++;
                                working_days--;
                            }
                        }
                    }
                
                
                // Partition relaxable variables: curriculum compactness (alert: works on curricula)
                float curriculum_compactness = ceil(((float) curriculum_compactness_cost.val() / all_cost) * free);
                
                cerr << " curriculum comp.: " << curriculum_compactness << endl;
                
                vector<unsigned int> lectures;
                for (unsigned int l = 0; l < in.TotalLectures(); l++)
                    if (find(all.begin(), all.end(), l) != all.end() && lecture_compactness[l].val())
                        lectures.push_back(l);
                
                random_shuffle(lectures.begin(), lectures.end());
                
                while (curriculum_compactness >= 1 && lectures.size())
                {
                    all.erase(find(all.begin(), all.end(), lectures.back()));
                    
                    lectures.pop_back();
                    freed++;
                    curriculum_compactness--;
                }

            }
            else
            {                
                bool chosen = false;
                
                // Choose random component to optimize (stochastically at random based on cost)
                double r = ((double)rand() / (double)RAND_MAX) * cost().val();
                double inc = 0;
                
                // Fall in room capacity?
                inc += room_capacity_cost.val() * ROOM_CAPACITY_COST;
                
                if (r < inc && !chosen)
                {
                    
                    //cerr << "Total freeable: " << free << " on ROOM CAPACITY." << endl;
                    double room_capacity = free;
                    
                    for(unsigned int l = 0; l < in.TotalLectures() && room_capacity >= 1; l++)
                        if (room_capacity_deviation[l].val() > 0 && find(all.begin(), all.end(), l) != all.end())
                        {
                            all.erase(find(all.begin(), all.end(), l));
                            freed++;
                            room_capacity--;
                        }
                    
                    chosen = true;
                }
                
                inc += room_stability_cost.val() * ROOM_STABILITY_COST;
                
                if (r < inc && !chosen)
                {
                    //cerr << "Total freeable: " << free << " on ROOM STABILITY." << endl;
                    double room_stability = free;
                    
                    vector<unsigned int> unstable_courses;
                    for(unsigned int c = 0; c < in.Courses(); c++)
                        if (room_stability_deviation[c].val() > 0)
                            unstable_courses.push_back(c);
                    
                    random_shuffle(unstable_courses.begin(), unstable_courses.end());
                    
                    for (unsigned int ci  = 0; ci < unstable_courses.size() && room_stability >= 1; ci++)
                    {
                        unsigned int c = unstable_courses[ci];
                        
                        // Check for room stability variables budget is performed outside to avoid splitting courses
                        for(int l = 0; l < in.CourseVector(c).Lectures();l++)
                        {
                            if (find(all.begin(), all.end(), index_of_start_lecture[c]+l) != all.end())
                            {
                                all.erase(find(all.begin(), all.end(), index_of_start_lecture[c]+l));
                                freed++;
                                room_stability--;
                            }
                        }
                    }

                    
                    chosen = true;
                }
            
                inc += curriculum_compactness_cost.val() * CURRICULUM_COMPACTNESS_COST;
                
                if (r < inc && !chosen)
                {
                    //cerr << "Total freeable: " << free << " on CURRICULUM COMPACTNESS." << endl;
                    double curriculum_compactness = free;
                    
                    vector<unsigned int> lectures;
                    for (unsigned int l = 0; l < in.TotalLectures(); l++)
                        if (find(all.begin(), all.end(), l) != all.end() && lecture_compactness[l].val())
                            lectures.push_back(l);
                    
                    random_shuffle(lectures.begin(), lectures.end());
                    
                    while (curriculum_compactness >= 1 && lectures.size())
                    {
                        all.erase(find(all.begin(), all.end(), lectures.back()));
                        
                        lectures.pop_back();
                        freed++;
                        curriculum_compactness--;
                    }

                    
                    chosen = true;
                }
                
                inc += minimum_working_days_cost.val() * MINIMUM_WORKING_DAYS_COST;
                
                if (r < inc && !chosen)
                {
                    //cerr << "Total freeable: " << free << " on WORKING DAYS." << endl;
                    double working_days = free;
                    
                    
                    for(unsigned int c = 0; c < in.Courses() && working_days >= 1; c++)
                        if (minimum_working_days_deviation[c].val() > 0)
                        {
                            vector<int> lectures;
                            for (int l = 0; l < in.CourseVector(c).Lectures(); l++)
                                lectures.push_back(index_of_start_lecture[c]+l);
                            
                            random_shuffle(lectures.begin(), lectures.end());
                            
                            for (int l : lectures)
                            {
                                if (find(all.begin(), all.end(), l) != all.end())
                                {
                                    if (working_days < 1)
                                        break;
                                    
                                    all.erase(find(all.begin(), all.end(), l));
                                    freed++;
                                    working_days--;
                                }
                            }
                        }
                    
                    chosen = true;
                }
            }
        }
        
        random_shuffle(all.begin(), all.end());

        int f = freed + free;
        // Release a total of <free> variables, plus a number of variables equals to the ones freed heuristically (<freed>)
        while (f > 0  && !all.empty())
        {
            all.pop_back();
            f--;
        }
        
        for (int s : all)
            rel(*tentative, tentative->roomslot[s] == roomslot[s].val());
        
        //if (violations() == 0)
        //    cerr << "Freed " <<  (int)roomslot.size() - (int)all.size() << "/" << free << endl;
        /*
        static unsigned int c = 0;
        if (c%10 == 0)
            cerr << " * to free: " << free << ", but really freed: " << (int)roomslot.size() - (int)all.size() << endl;
        c++;
        */
        
        return (int)roomslot.size() - (int)all.size();
        
    }
    
    /** Specialized constrain function with lexicographic optimization */
    virtual void constrain(const Space& b, int delta)
    {
        const LNSCBCTT& cb = static_cast<const LNSCBCTT&>(b);
        
        if (cb.violations())
        {
            // If there are any conflicts, reduce them or leave them in
            if (cb.conflicts.val() == 0)
                post_hard_conflicts();
            else
            {
                if (cb.duplicates.val())
                    rel(*this, conflicts <= cb.conflicts.val());
                else
                    rel(*this, conflicts < cb.conflicts.val());
            }
            
            // If there are any duplicates, reduce or leave them in
            if (cb.duplicates.val() < in.TotalLectures())
                rel(*this, duplicates > cb.duplicates.val());
            else
                post_hard_duplicates();
         
            // BUT, overall reduce violations
            // rel(*this, (duplicates - in.TotalLectures()) + conflicts < cb.violations() );
        }
        else
        {
            post_hard_constraints();            
            rel(*this, z < cb.cost().val() + delta);
        }
    }
    
    /** Post all hard constraints */
    virtual void post_hard_constraints()
    {
        post_hard_conflicts();
        post_hard_duplicates();
    }
    
    /** Total violations, i.e. what makes the solution unfeasible */
    virtual unsigned int violations() const
    {
        return (in.TotalLectures() - duplicates.val()) + conflicts.val();
    }
    
    Space* copy(bool share)
    {
        return new LNSCBCTT(share, *this);
    }
};


#endif
