#ifndef CP_CTT_LNSSpace_hh
#define CP_CTT_LNSSpace_hh

#include <gecode/driver.hh>
#include <gecode/gist.hh>

using namespace Gecode;

/**
 Base template class for spaces that must be handled with LNS
 @remarks method relax(double) must be re-implemented to specialize the template 
 */
template <class T>
class LNSSpace : public T
{
public:
         
    /** Gecode constructor (for drivers, etc.) */
    LNSSpace<T>(const InstanceOptions& o) : T(o) { }
    
    /** Gecode copy constructor */
    LNSSpace<T>(bool share, LNSSpace<T>& t) : T(share, t) { }
    
    /** Method to generate a relaxed solution from the current one */
    virtual unsigned int relax(LNSSpace<T>* tentative, unsigned int free) = 0;
    
    /** Returns the number of relaxable variables */
    virtual unsigned int relaxable_vars() = 0;
    
    /** Returns a variable which represents the presence of violations */
    virtual unsigned int violations () const = 0;
    
    /** Post constraints that are not part of the cost function */
    virtual void post_hard_constraints() = 0;
    
    virtual Space* copy(bool share) = 0;
};

/**
 A class in which branching is posted separately from the model 
 @remarks remarks
 */
template <class T>
class DeferredBranchingSpace : public T
{
public:
    
    /** Default constructor, needed when T = Minimize/MaximizeScript */
    DeferredBranchingSpace<T>() : T() {  }
    
    /** Gecode constructor (for drivers, etc.) */
    DeferredBranchingSpace<T>(const InstanceOptions& o) : T(o) { }
    
    /** Gecode copy constructor */
    DeferredBranchingSpace<T>(bool share, DeferredBranchingSpace<T>& t) : T(share, t) { }
    
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
    
    /** Post a random branching, e.g. good for finding an initial solution */
    virtual void PostRandomBranching() { };
    
    /** Post a branching likely to find a good solution  */
    virtual void PostBestBranching() { };
        
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
        this->PostBestBranching();
    }
    
    /** Gecode copy constructor (branching already posted) */
    InstantBranchingSpace<T>(bool share, InstantBranchingSpace<T>& t) : T(share, t) { }
    
    /** Gecode copy method */
    virtual Space* copy(bool share)
    {
        return new InstantBranchingSpace<T>(share, *this);
    }
};

#endif
