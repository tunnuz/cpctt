// File Faculty.hpp
#ifndef FACULTY_HPP
#define FACULTY_HPP

#include <string>
#include <vector>
#include <map>
#include <list>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cassert>
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif


#define HARD_WEIGHT_SET
const int HARD_WEIGHT = 1;

using namespace std;
 
const int BUF_SIZE = 200;

enum Priority { impossible = -2, undesired, normal, desired, imposed};

class Course
{
  friend ostream& operator<<(ostream&, const Course&);
  friend istream& operator>>(istream&, Course&);
public:
  Course() {} // data are initialized by the operator >>
  const string& Name() const { return name; }
  const string& Teacher() const { return teacher; }
  unsigned Students() const { return students; }
  void AddStudents(unsigned s) {  students += s; }
  unsigned Lectures() const { return lectures; }
  unsigned MinWorkingDays() const { return min_working_days; }
  void SetDoubleLectures(Priority p) { double_lectures = p; }
  Priority DoubleLectures() const { return double_lectures; }
  void ReadCourse(istream& is); // in CTT format (instead of ECTT)
protected:
  string name, teacher;
  unsigned lectures, students, min_working_days;
  Priority double_lectures;
};

class Curriculum // curricula
{  
friend ostream& operator<<(ostream&, const Curriculum&);
public:
  Curriculum() {}
  const string& Name() const { return name; }
  void SetName(const string& n) { name = n; }
  unsigned Size() const { return members.size(); }
  //  void Clear() { members.clear(); }
  void AddMember(unsigned e) { members.push_back(e); }
  void AddCourse(string c) { courses.push_back(c); }
  //  bool IsMember(unsigned e) const {  }
  unsigned operator[](unsigned i) const { return members[i]; }
  //protected:
  string name;
  vector<unsigned> members;
  vector<string> courses; // these are the names of the courses included in the curriculum
};

class Room
{
  friend ostream& operator<<(ostream&, const Room&);
  friend istream& operator>>(istream&, Room&);
public:
  Room() {}
  Room(const string& n, unsigned c, unsigned l) { name = n; capacity = c; location = l; }
  const string& Name() const { return name; }
  unsigned Capacity() const { return capacity; }
  unsigned Location() const { return location; }
  void SetLocation(unsigned l)  { location = l; }
  void SetCapacity(unsigned c)  { capacity = c; }
  //protected:
  string name;
  unsigned capacity;
  unsigned location;
};


class Faculty
{
  friend ostream& operator<<(ostream&, const Faculty&);
public:
  Faculty();
  Faculty(const string& filename);
  void Read(const string& filename);
  void ReadFromECTT(const string& filename);
  void ReadFromCTT(const string& filename);
  unsigned Courses() const { return courses; }
  unsigned Rooms() const { return rooms; }
  unsigned Periods() const { return periods; }
  unsigned PeriodsPerDay() const { return periods_per_day; }
  unsigned MorningPeriods() const { return morning_periods; }
  unsigned Days() const { return periods/periods_per_day; }
  unsigned TotalLectures() const { return total_lectures; }
  unsigned DummyRoom() const { return rooms + 1; }

  unsigned MinLectures() const { return min_lectures; }
  unsigned MaxLectures() const { return max_lectures; }  

  bool  Available(unsigned c, unsigned p) const 
  { return availability[c][p]; } // availability matrix access
  bool Conflict(unsigned c1, unsigned c2) const 
  { return conflict[c1][c2]; } // conflict matrix access
  const Course& CourseVector(int i) const { return course_vect[i]; }
  const Room& RoomVector(int i) const { return room_vect[i]; }
  const Curriculum& CurriculaVector(int i) const { return curricula_vect[i]; }
  
  bool CurriculumMember(unsigned c, unsigned g) const { return course_curriculum_membership[c][g]; }
  unsigned CourseConflicts(unsigned c) const { return conflict_list[c].size(); }
  unsigned CourseConflict(unsigned c, unsigned a) const { return conflict_list[c][a]; }

  unsigned CourseCurricula(unsigned c) const { return curricula_list[c].size(); }
  unsigned CourseCurriculum(unsigned c, unsigned a) const { return curricula_list[c][a]; }

  unsigned LectureCourse(unsigned l) const { return lecture_position[l].first; }
  unsigned LecturePosition(unsigned l) const { return lecture_position[l].second; }


  Priority RoomPreference(unsigned c, unsigned r) const { return room_preference[c][r]; } 
//   Priority RoomAvailability(unsigned r, unsigned p) const { return room_availability[r][p]; } 

  int RoomIndex(const string&) const; 
  int CourseIndex(const string&) const;
  int CurriculumIndex(const string&) const;
  //  int PeriodIndex(const string&) const;
  unsigned Curricula() const { return curricula; }
  //  const string& DirName() const { return dir_name; }
  const string& Name() const { return name; }

  // they should be const, but at present...
  unsigned MIN_WORKING_DAYS_COST;
  unsigned COMPACTNESS_COST;
  unsigned ISOLATED_LECTURES_COST;
  unsigned STABILITY_COST;
  unsigned DOUBLE_LECTURES_COST;
  unsigned PREFERENCE_COST;
  unsigned TRAVEL_COST;
  unsigned STUDENT_LOAD_COST;

protected:

  void PrintStatistics(ostream& os) const;
  unsigned ComputeSeatOveruse() const;

  void AddConflict(unsigned c1, unsigned c2);
  
  void CheckFeasibility() const;

  string name; 
  unsigned rooms, courses, periods, periods_per_day, curricula;
  unsigned total_lectures; // Total number of lectures for all courses
  unsigned min_lectures, max_lectures; // Used by CC StudentLoad
  unsigned morning_periods; // Used by CC LunchBreak
  

  // data objects
  vector<Course> course_vect;
  vector<Room> room_vect;

  // availability and conflicts constraints
  vector<vector<bool> > availability;
  vector<vector<bool> > conflict;

  // course curricula
  vector<Curriculum> curricula_vect;

  // room preference for courses
  vector<vector<Priority> > room_preference;

//   // room-period availability
//   vector<vector<Priority> > room_availability; 

  // lists for accelerating access
  vector<vector<unsigned> > conflict_list;
  vector<vector<unsigned> > curricula_list;
  vector<vector<bool> > course_curriculum_membership;
  vector<pair<unsigned,unsigned> > lecture_position; // vector of size total_lectures: for each lecture gives the course and the number of order 
};

class Timetable
{
  friend ostream& operator<<(ostream&, const Timetable&);
  friend istream& operator>>(istream&, Timetable&);
public:
  Timetable(const Faculty & f); 
  Timetable(const Timetable& t);
  Timetable& operator=(const Timetable& t);
	
  virtual ~Timetable() {} 

  int Load2(const string file_name);
  void Write2(string instance); 

    // matrix access functions (const and non-const)
  unsigned operator()(unsigned i, unsigned j) const { return T[i][j]; }
  unsigned& operator()(unsigned i, unsigned j) { return T[i][j]; }
  void CheckFeasibility() const; // checks whether a timetable (read from file) is feasible
 protected:
  const Faculty & in;  
  vector<vector<unsigned> > T;  // (courses X periods) timetable matrix
};


#endif
