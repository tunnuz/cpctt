#ifndef CP_CTT_branching_hh
#define CP_CTT_branching_hh

/**
 A class template representing a Space whose branching is posted separately from the model.
 The template class (T) should redefine the basic branching styles.
 @remarks the cost variable is "cost".
 */
template <class T>
class DeferredBranchingSpace : public T
{
public:
  
  /** Default constructor, needed when T = Minimize/MaximizeScript */
  DeferredBranchingSpace<T>() : T()
  { }
  
  /** Gecode constructor (for drivers, etc.) */
  DeferredBranchingSpace<T>(const InstanceOptions& o) : T(o)
  { }
  
  /** Gecode copy constructor */
  DeferredBranchingSpace<T>(bool share, DeferredBranchingSpace<T>& t) : T(share, t)
  { }
  
  /** Gecode copy method */
  virtual Space* copy(bool share)
  {
    return new DeferredBranchingSpace<T>(share, *this);
  }
  
  /** Needed when T = Minimize/MaximizeScript */
  virtual IntVar cost() const
  {
    return this->cost();
  }
  
  /** Post a branching good for performing a typical tree search, e.g., B&B, DFS, ... */
  virtual void tree_search_branching() {  }
  
};

/**
 A class that takes a DeferredBranchingSpace as a template parameter and just posts the branching straight away
 */
template <class T>
class InstantBranchingSpace : public T
{
public:
  
  // Gecode default constructor
  InstantBranchingSpace<T>(const InstanceOptions& o) : T(o)
  {
    this->tree_search_branching();
  }
  
  /** Gecode copy constructor (branching already posted) */
  InstantBranchingSpace<T>(bool share, InstantBranchingSpace<T>& t) : T(share, t)
  { }
  
  /** Gecode copy method */
  virtual Space* copy(bool share)
  {
    return new InstantBranchingSpace<T>(share, *this);
  }
};

#endif
