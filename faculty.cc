// File faculty.cpp
#include "faculty.hh"
#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <algorithm>

ostream& operator<<(ostream& os, const Course& c)
{
  os << c.name << " " << c.teacher << " " << c.lectures << " "
  << c.min_working_days << " " << c.students;
  if (c.double_lectures == desired)
    os << " 1";
  else
    os << " 0";
  return os;
}

void Course::ReadCourse(istream& is)
{
  is >> name >> teacher >> lectures >> min_working_days >> students;
  double_lectures = normal;
}

istream& operator>>(istream& is, Course& c)
{
  unsigned dl;
  is >> c.name >> c.teacher >> c.lectures >> c.min_working_days >> c.students >> dl;
  if (dl == 0)
    c.double_lectures = normal;
  else
    c.double_lectures = desired;
  return is;
}

ostream& operator<<(ostream& os, const Curriculum& g)
{
  os << g.Name() << " " << g.Size();
  return os;
}

ostream& operator<<(ostream& os, const Room& r)
{
  return os << r.name << " " << r.capacity << " " << r.location;
}

istream& operator>>(istream& is, Room& r)
{
  char buffer[BUF_SIZE];
  is >> r.name >> r.capacity >> r.location;
  is.getline(buffer,BUF_SIZE);
  return is;
}

// **************************************************************
// ********************  TIMETABLE ********************************
// ************************************************************** 

ostream& operator<<(ostream& os, const Timetable& tt)
{
  unsigned i, j, r;
  string course_name, room_name;
  
  for (i = 0; i < tt.T.size(); i++)
  {
    for (j = 0; j < tt.T[i].size(); j++)
    {
      r = tt.T[i][j];
      if (r != 0)
        os << tt.in.CourseVector(i).Name() << ' ' << tt.in.RoomVector(r).Name() << ' ' 
        << j / tt.in.PeriodsPerDay() << ' ' << j % tt.in.PeriodsPerDay() << endl;
    }
  }
  return os; 
}

istream& operator>>(istream& is, Timetable& tt)
{ 
  unsigned i, j, day, period;
  int room;
  string course_name, room_name;
  
  for (i = 0; i < tt.T.size(); i++)
  {
    for (j = 0; j < tt.T[i].size(); j++)
      tt.T[i][j] = 0;
    for (j = 0; j < tt.in.CourseVector(i).Lectures(); j++)
    {
      is >> course_name >> room_name >> day >> period;	  
      assert (course_name == tt.in.CourseVector(i).Name());
      room = tt.in.RoomIndex(room_name);
      tt.T[i][day * tt.in.PeriodsPerDay() + period] = room;
    }
  }
  tt.CheckFeasibility();
  return is; 
}

void Timetable::CheckFeasibility() const
{
  bool feasible = true;
  unsigned p, c, r, lectures;
  for (c = 0; c < in.Courses(); c++)
  {
    lectures = 0;
    for (p = 0; p < in.Periods(); p++)
    {
      r = T[c][p];
      if (r != 0)
	    {
	      lectures++;
	      if (!in.Available(c,p))
        {
          cerr << "Lecture of " << in.CourseVector(c).Name() << " at period"
			    << p << endl;
          feasible = false;
        }
	    }
    }
    if (lectures != in.CourseVector(c).Lectures())
    {
      cerr << "Wrong number of lectures for " << in.CourseVector(c).Name() << endl;
      feasible = false;
    }
  }
  if (!feasible)
    throw std::logic_error("Timetable is not feasible");
}

Timetable::Timetable(const Faculty &f)
: in(f),   T(in.Courses(), vector<unsigned>(in.Periods(),0))
{}

Timetable::Timetable(const Timetable &t)
: in(t.in), T(t.T)
{}

Timetable& Timetable::operator=(const Timetable &t)
{ 
  T = t.T; 
  return *this;
}

// **************************************************************
// ********************  FACULTY ********************************
// ************************************************************** 

Faculty::Faculty()
{
  rooms = courses = periods = periods_per_day = curricula = total_lectures = min_lectures = max_lectures = morning_periods = 0;
}


Faculty::Faculty(const string& file_name)
{
  Read(file_name);	
}

void Faculty::Read(const string& file_name)
{	
  if (file_name.find(".ectt") != string::npos)
    ReadFromECTT(file_name);
  else if (file_name.find(".ctt") != string::npos)
    ReadFromCTT(file_name);
  else
    throw runtime_error("Unknown input format (must be .ectt or .ctt)");
  CheckFeasibility();
}

void Faculty::CheckFeasibility() const
{
  // FIXME: further checks should be provided (this is just to prevent bad number of lectures)
  unsigned c, p, possible_periods;
  for (c = 0; c < courses; c++)
  {
    possible_periods = 0;
    for (p = 0; p < periods; p++)
    {
      if (availability[c][p])
        possible_periods++;
    }
    if (possible_periods < course_vect[c].Lectures())
    {
      std::cerr << "Number of possible lectures for course " << course_vect[c].Name() << " not feasible ("
      << "expecting at least " << course_vect[c].Lectures() << ", found " << possible_periods << ")" << std::endl;
      throw std::logic_error("Input is not feasible");
    }
  }
}

void Faculty::ReadFromECTT(const string& file_name)
{// legge dal nuovo formato (quello monofile)
  string curriculum, course_name, room_name, period_name, 
  teacher_name, priority;
  string course_name1, course_name2;
  unsigned curriculum_size, days, unavail_constraints, room_constraints;
  unsigned c, c1, c2, r, cu, p, l, period_index, day_index;
  char buffer[BUF_SIZE];
  
  ifstream is(file_name.c_str());
  if (!is)
    throw std::logic_error("Could not open instance file " + file_name);
  
  total_lectures = 0;
  is >> buffer;
  is.get(); // eat the blank after Name:
  is.getline(buffer,BUF_SIZE);
  name = buffer;
  
  is >> buffer >> courses;
  is >> buffer >> rooms;
  is >> buffer >> days;
  is >> buffer >> periods_per_day;
  is >> buffer >> curricula;
  is >> buffer >> min_lectures >> max_lectures;
  is >> buffer >> unavail_constraints;
  is >> buffer >> room_constraints;
  
  periods = days * periods_per_day;
  morning_periods = periods_per_day/2; // To be read from file in general
  
  // **********************************
  // Allocate vectors and matrices
  // **********************************
  course_vect.resize(courses);
  //   period_vect.resize(periods);
  // location 0 of room_vect is nt used (teaching in room 0 means NOT TEACHING)
  room_vect.resize(rooms + 2);  // one for the Dummy room
  curricula_vect.resize(curricula);
  
  availability.resize(courses, vector<bool>(periods,true));
  conflict.resize(courses, vector<bool>(courses));
  
  conflict_list.resize(courses);
  curricula_list.resize(courses);
  course_curriculum_membership.resize(courses, vector<bool>(curricula,false));
  
  room_preference.resize(courses,vector<Priority>(rooms+2,normal));
  //  room_availability.resize(rooms+2,vector<Priority>(periods,normal));
  
  // **********************************
  // Read courses
  // **********************************
  
  is >> buffer;
  for (c = 0; c < courses; c++)
  {
    is >> course_vect[c];
    total_lectures += course_vect[c].Lectures();
    for (l = 0; l < course_vect[c].Lectures(); l++)
      lecture_position.push_back(make_pair(c,l));
  }
  
  // **********************************
  // Read rooms
  // **********************************
  
  is >> buffer;
  for (r = 1; r <= rooms; r++)
    is >> room_vect[r];
  room_vect[rooms+1] = Room("*",1000,100); // capacity 1000 (any lecture), site 100 (no site)
  
  // **********************************
  // Read curricula
  // **********************************
  
  is >> buffer;
  for (cu = 0; cu < curricula; cu++)
  {
    is >> buffer >> curriculum_size;
    curricula_vect[cu].SetName(buffer);
    unsigned i1, i2;
    for (i1 = 0; i1 < curriculum_size; i1++)
    {
      is >> course_name;
      c1 = (unsigned) CourseIndex(course_name);	  
      curricula_vect[cu].AddMember(c1);
      course_curriculum_membership[c1][cu] = true;
      curricula_list[c1].push_back(cu);
      for (i2 = 0; i2 < i1; i2++)
	    {
	      c2 = curricula_vect[cu][i2];
	      AddConflict(c1,c2);
	    }
    }
    is.getline(buffer,BUF_SIZE); // discards the rest of the line
  }
  
  
  // **********************************
  // Read constraints
  // **********************************
  
  is >> buffer;
  
  // Courses -- Periods
  for (unsigned i = 0; i < unavail_constraints; i++)
  {
    is >> course_name  >> day_index >> period_index;
    p = day_index * periods_per_day +  period_index;
    c = CourseIndex(course_name);
    availability[c][p] = false;
  }
  
  is >> buffer;
  
  // Courses -- Rooms
  for (unsigned i = 0; i < room_constraints; i++)
  {
    is >> course_name  >> room_name;
    c = CourseIndex(course_name);
    r = RoomIndex(room_name);
    room_preference[c][r] = undesired;
  }
  
  // **********************************
  // Add same-teacher constraints
  // **********************************
  
  for (c1 = 0; c1 < courses - 1; c1++)
    for (c2 = c1+1; c2 < courses; c2++)
      if (course_vect[c1].Teacher() == course_vect[c2].Teacher())
        AddConflict(c1,c2);
  
}

void Faculty::ReadFromCTT(const string& file_name)
{// legge dal nuovo formato (quello monofile)
  string curriculum, course_name, room_name, period_name, 
  teacher_name, priority;
  string course_name1, course_name2;
  unsigned curriculum_size, days, unavail_constraints;
  unsigned c, c1, c2, r, cu, p, l, period_index, day_index;
  char buffer[BUF_SIZE];
  
  ifstream is(file_name.c_str());
  if (!is)
    throw std::logic_error("Could not open instance file " + file_name);
  
  total_lectures = 0;
  is >> buffer;
  is.get(); // eat the blank after Name:
  is.getline(buffer,BUF_SIZE);
  name = buffer;
  
  is >> buffer >> courses;
  is >> buffer >> rooms;
  is >> buffer >> days;
  is >> buffer >> periods_per_day;
  is >> buffer >> curricula;
  is >> buffer >> unavail_constraints;
  
  periods = days * periods_per_day;
  morning_periods = periods_per_day/2; // To be read from file in general
  
  // **********************************
  // Allocate vectors and matrices
  // **********************************
  course_vect.resize(courses);
  //   period_vect.resize(periods);
  // location 0 of room_vect is nt used (teaching in room 0 means NOT TEACHING)
  room_vect.resize(rooms + 2);  // one for the Dummy room
  curricula_vect.resize(curricula);
  
  availability.resize(courses, vector<bool>(periods,true));
  conflict.resize(courses, vector<bool>(courses));
  
  conflict_list.resize(courses);
  curricula_list.resize(courses);
  course_curriculum_membership.resize(courses, vector<bool>(curricula,false));
  
  room_preference.resize(courses,vector<Priority>(rooms+2,normal));
  //  room_availability.resize(rooms+2,vector<Priority>(periods,normal));
  
  // **********************************
  // Read courses
  // **********************************
  
  is >> buffer;
  for (c = 0; c < courses; c++)
  {
    course_vect[c].ReadCourse(is);
    total_lectures += course_vect[c].Lectures();
    for (l = 0; l < course_vect[c].Lectures(); l++)
      lecture_position.push_back(make_pair(c,l));
  }
  
  // **********************************
  // Read rooms
  // **********************************
  
  is >> buffer;
  for (r = 1; r <= rooms; r++)
    is >> room_vect[r].name >> room_vect[r].capacity;
  room_vect[rooms+1] = Room("*",1000,100); // capacity 1000 (any lecture), site 100 (no site)
  
  // **********************************
  // Read curricula
  // **********************************
  
  is >> buffer;
  for (cu = 0; cu < curricula; cu++)
  {
    is >> buffer >> curriculum_size;
    curricula_vect[cu].SetName(buffer);
    unsigned i1, i2;
    for (i1 = 0; i1 < curriculum_size; i1++)
    {
      is >> course_name;
      c1 = (unsigned) CourseIndex(course_name);	  
      curricula_vect[cu].AddMember(c1);
      course_curriculum_membership[c1][cu] = true;
      curricula_list[c1].push_back(cu);
      for (i2 = 0; i2 < i1; i2++)
	    {
	      c2 = curricula_vect[cu][i2];
	      AddConflict(c1,c2);
	    }
    }
    is.getline(buffer,BUF_SIZE); // discards the rest of the line
  }
  
  
  // **********************************
  // Read constraints
  // **********************************
  
  is >> buffer;
  
  // Courses -- Periods
  for (unsigned i = 0; i < unavail_constraints; i++)
  {
    is >> course_name  >> day_index >> period_index;
    p = day_index * periods_per_day +  period_index;
    c = CourseIndex(course_name);
    availability[c][p] = false;
  }
  
  // **********************************
  // Add same-teacher constraints
  // **********************************
  
  for (c1 = 0; c1 < courses - 1; c1++)
    for (c2 = c1+1; c2 < courses; c2++)
      if (course_vect[c1].Teacher() == course_vect[c2].Teacher())
        AddConflict(c1,c2);
  
}

void Faculty::AddConflict(unsigned c1, unsigned c2)
{
  if (!conflict[c1][c2])
  {
    conflict[c1][c2] = true;
    conflict[c2][c1] = true;  
    conflict_list[c1].push_back(c2);
    conflict_list[c2].push_back(c1);
  }
}      

ostream& operator<<(ostream& os, const Faculty& f)
{
  unsigned i, j;
  os << f.name << endl;
  os << "Courses : " << f.courses << endl;
  os << "Rooms : " << f.rooms << endl;
  os << "Periods : " << f.periods << " (" 
  << f.periods_per_day << " per day)" << endl;
  os << endl;
  
  os << "Courses: " << endl;
  for (i = 0; i < f.course_vect.size(); i++)
    os << f.course_vect[i] << endl;
  os << endl;
  
  os << "Rooms: " << endl;
  for (i = 1; i < f.room_vect.size(); i++)
    os << f.room_vect[i] << endl;
  os << endl;
  
  //   os << "Periods: " << endl;
  //   for (i = 0; i < f.period_vect.size(); i++)
  //     os << f.period_vect[i] << endl;
  //   os << endl;
  
  os << "Curricula: " << endl;
  for (i = 0; i < f.curricula; i++)
  {
    os << f.curricula_vect[i].Name() << " :  ";
    for (j = 0; j < f.curricula_vect[i].Size(); j++)
      os << f.course_vect[f.curricula_vect[i][j]].Name() << " ";
    os << endl;
  }
  os << endl;
  
  os << "Curricula lists: " << endl;
  for (i = 0; i < f.courses; i++)
  {
    os << i << " : ";
    for (unsigned j = 0; j < f.CourseCurricula(i); j++)
      os << f.CourseCurriculum(i,j) << " ";
    os << endl;
  }
  os << endl;
  
  os << "Conflicts: " << endl;
  for (i = 0; i < f.conflict.size(); i++)
  {
    for (j = 0; j < f.conflict[i].size(); j++)
      if (f.conflict[i][j])
        os << "X ";
      else 
        os << "- ";
    os << endl;
  }
  os << endl;
  
  os << "Conflict lists: " << endl;
  for (i = 0; i < f.courses; i++)
  {
    os << i << " : ";
    for (unsigned j = 0; j < f.CourseConflicts(i); j++)
      os << f.CourseConflict(i,j) << " ";
    os << endl;
  }
  os << endl;
  
  
  os << "Course <--> Period Constraint: " << endl;
  for (i = 0; i < f.availability.size(); i++)
  {
    for (j = 0; j < f.availability[i].size(); j++)
    {
      if (f.availability[i][j])
        os << "- ";
      else
        os << "X ";
    }
    os << endl;
  }
  os << endl;
  
//   os << "room <--> Period Constraint: " << endl;
//   for (i = 0; i < f.room_availability.size(); i++)
//   {
//     for (j = 0; j < f.room_availability[i].size(); j++)
//     {
//       if (f.room_availability[i][j] == normal)
//         os << "- ";
//       else
//         os << "X ";
//     }
//     os << endl;
//   }
//   os << endl;

  f.PrintStatistics(os);

  return os;
}

void Faculty::PrintStatistics(ostream& os) const
{
  unsigned c, c2, r, p, q;
  unsigned course_conflicts = 0, course_pairs = 0, lecture_conflicts = 0, lecture_pairs = 0, 
    total_students = 0, total_seats = 0, 
    total_teacher_availability_per_course = 0, total_teacher_availability_per_lecture = 0,  teacher_availability_per_course,
    total_room_suitability_per_lecture = 0, total_room_suitability_per_course = 0, days = periods/periods_per_day,
    seat_overuse;

  unsigned curriculum_lectures, min_curriculum_lectures = 0, max_curriculum_lectures = 0, total_curriculum_lectures = 0;

  for (c = 0; c < courses; c++)
    {
      teacher_availability_per_course = 0;
      total_students += course_vect[c].Lectures() * course_vect[c].Students();
      for (p = 0; p < periods; p++)
	{
	  total_teacher_availability_per_course += unsigned(availability[c][p]);
	  teacher_availability_per_course += unsigned(availability[c][p]);
	  total_teacher_availability_per_lecture += unsigned(availability[c][p]) * course_vect[c].Lectures();
	}
      if (teacher_availability_per_course < course_vect[c].Lectures())
	{
	  cerr << "Course " << course_vect[c].Name() << " has only " << teacher_availability_per_course << " periods available (" << course_vect[c].Lectures() << " necessary)" << endl;
	  assert (false);
	}
	
    }

  for (r = 1; r <= rooms; r++)
    {
      total_seats += room_vect[r].Capacity();
      for (c = 0; c < courses; c++)
	{
	  if (room_vect[r].Capacity() >= course_vect[c].Students() && room_preference[c][r] != impossible)
	    {
	      total_room_suitability_per_course++;
	      total_room_suitability_per_lecture += course_vect[c].Lectures();
	    }
	}
    }

  for (c = 0; c < courses-1; c++)
    for (c2 = c+1; c2 < courses; c2++)
      {
	if (conflict[c][c2])
	  {
	    course_conflicts++;
	    lecture_conflicts += course_vect[c].Lectures() * course_vect[c2].Lectures();
	  }
      }

  for (c = 0; c < courses; c++)
    lecture_conflicts += course_vect[c].Lectures() * (course_vect[c].Lectures() - 1) / 2;

  for (q = 0; q < curricula; q++)
    {
      curriculum_lectures = 0;
      for (c = 0; c < curricula_vect[q].Size(); c++)
	curriculum_lectures += course_vect[curricula_vect[q][c]].Lectures();
      total_curriculum_lectures += curriculum_lectures;
      if (q == 0 || curriculum_lectures < min_curriculum_lectures)
	min_curriculum_lectures = curriculum_lectures;
      if (q == 0 || curriculum_lectures > max_curriculum_lectures)
	max_curriculum_lectures = curriculum_lectures;
    }

  course_pairs = courses * (courses - 1) / 2;
  lecture_pairs = total_lectures * (total_lectures - 1) / 2;

  seat_overuse = ComputeSeatOveruse();


  os << "Scalar data: courses = " << courses << ", total lectures = " << total_lectures << ", rooms = " << rooms << ", periods_per_day = " 
     << periods_per_day << ", days = " << days << ", curricula = " << curricula 
     << ", Daily lectures = " << min_lectures << "-" << max_lectures << endl;
  os << "Statistics: (per course/per lecture)" << endl;
  
  os << "Conflict Density = " 
     << (100.0 * course_conflicts)/course_pairs << "%/" 
     << (100.0 * lecture_conflicts)/lecture_pairs << "%" << endl;
      
  os << "Teachers' Availability = " << (100.0 * total_teacher_availability_per_course) / (courses * periods) << "%/" 
     << (100.0 * total_teacher_availability_per_lecture) / (total_lectures * periods) << "%" << endl;
  
  os << "Rooms' suitability (availability + capacity): = " 
     << (100.0 * total_room_suitability_per_course) / (rooms * courses) << "%/" 
     << (100.0 * total_room_suitability_per_lecture) / (rooms * total_lectures) << "%"
     << endl;
  
  os << "Curriculum lectures per day: "
     << "Min = " << min_curriculum_lectures/(float)days 
     << ", Avg = " << total_curriculum_lectures/((float)days * curricula) 
     << ", Max = " << max_curriculum_lectures/(float)days 
     << endl;
  
  os << "Room occupation : per room " << (100.0 * total_lectures) / (rooms * periods) << "%, "
     << " per seat " << (100.0 * total_students) / (total_seats * periods) << "%" << endl;

  os << "Features:" << courses << "," << total_lectures << "," << rooms << "," << periods << "," << curricula << "," << (100.0 * total_lectures) / (rooms * periods) 
     << "," << (100.0 * lecture_conflicts)/lecture_pairs
     << "," << (100.0 * total_teacher_availability_per_lecture) / (total_lectures * periods)
     << "," << (100.0 * total_room_suitability_per_lecture) / (rooms * total_lectures)
     << "," << total_curriculum_lectures/((float)days * curricula) 
     << endl;

  os << "Seat overuse: " << seat_overuse << endl;
}

unsigned Faculty::ComputeSeatOveruse() const
{
  unsigned rs, r, p, c, l, ls;
  unsigned over_use = 0;
  vector<unsigned> roomslot_size(rooms * periods, 0), lecture_size(total_lectures, 0);
  
  rs = 0;
  for (r = 1; r <= rooms; r++)
    for (p = 0; p < periods; p++)
      {
	roomslot_size[rs] = room_vect[r].Capacity();
	rs++;
      }

  ls = 0;
  for (c = 0; c < courses; c++)
    for (l = 0; l < course_vect[c].Lectures(); l++)
      {
	lecture_size[ls] = course_vect[c].Students();
	ls++;
      }
  sort(roomslot_size.begin(), roomslot_size.end(), std::greater<unsigned>());
  sort(lecture_size.begin(), lecture_size.end(), std::greater<unsigned>());

  for (ls = 0; ls < lecture_size.size(); ls++)
    if (lecture_size[ls] > roomslot_size[ls])
      over_use += lecture_size[ls] - roomslot_size[ls];
  return over_use;
}

int Faculty::CourseIndex(const string& name) const
{
  for (unsigned i = 0; i < course_vect.size(); i++)
    if (course_vect[i].Name() == name)
      return i;
  return -1;
}

int Faculty::CurriculumIndex(const string& name) const
{
  for (unsigned i = 0; i < curricula_vect.size(); i++)
    if (curricula_vect[i].Name() == name)
      return i;
  return -1;    
}


int Faculty::RoomIndex(const string& name) const
{
  for (unsigned i = 0; i < room_vect.size(); i++)
    if (room_vect[i].Name() == name)
      return i;
  return -1;
}

