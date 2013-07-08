#ifndef CP_CTT_LNS_hh
#define CP_CTT_LNS_hh

// #include "CBCTT.hh"
#include <vector>
#include <algorithm>
#include <set>

#include <gecode/kernel.hh>
#include <gecode/search.hh>
#include <gecode/search/support.hh>
#include <gecode/driver.hh>
#include <gecode/search/worker.hh>
#include <gecode/search/sequential/path.hh>

#define PRINT_INTERMEDIATE 1
#undef PRINT_INTERMEDIATE

using namespace Gecode;
using namespace std;

namespace Gecode
{
    namespace Search
    {
        namespace Sequential
        {
            
            /** Real search engine implementation (at the bottom of the file there
             is the proxy implementation, needed for scripts */
            template <class T>
            class LNS : public Worker {
                
            protected:
                
                /** Root of the search tree */
                Space* root;
                
                /** Search options */
                Options opt;
                
                /** Statistics. */
                Statistics stats;
                
                /** Iteration counter. */
                int idle_iterations;
                
                T* best;
                
                /** Current solution. */
                T* cur;
                
            public:
                
                /** Maximum number of idle iterations (not used right now) */
                static unsigned int max_idle_iterations;
                
                /** Milliseconds per variable B&B */
                static unsigned int ms_per_var;
                
                /** Free variables (starting value) */
                static unsigned int free_vars;
                
                /** Free variables (maximum value) */
                static double max_free_vars;
                
                /** Threads used (by B&B) */
                static unsigned int threads;
                
                /** How often do we do a completely random relaxation. */
                static double random_relaxation;
                
                /** Just a temporary bool variable in an accessible place. */
                static bool random;
                
                /** Constrain solution? */
                static bool constrain;

                /** Initial temperature. */
                static double temperature;

                /** After how many acceptances we lower the temperature */
                static int neighbors_accepted;

                /** Minimum temperature (default = 0.4, with p = 0.05 yields an expected delta of 1) */
                static double min_temperature;

                /** Cooling rate */
                static double cooling_rate;


                /** Probability of accepting worse solutions */
                static double p;

                /** Constructor
                 @param s starting space (not propagated)
                 @param sz size of the search space
                 @param o search options
                 */
                LNS(Space* s, size_t sz, const Search::Options& o);
                
                /** Return next solution (return NULL if search has stopped or there are no feasible solutions) */
                Space* next(void);
                
                /** Return search stats */
                Statistics statistics(void) const { return stats; }
                
                /** Destructor */
                ~LNS(void);
            };
            
            template <class T>
            forceinline
            LNS<T>::LNS(Space* s, size_t sz, const Search::Options& o) :
            Worker(sz), root(s->status() == SS_FAILED ? NULL : s->clone()), opt(o), idle_iterations(0), best(NULL)
            {
                // cur solution is the root
                stats = *this;
            }
            
            template <class T>
            forceinline
            Space* LNS<T>::next(void)
            {

                // Start worker
                start();

                // Get number of threads from options
                threads = opt.threads;

                // Try solving the full problem and get a feasible solution
                T* full_initial = static_cast<T*>(root->clone());
                full_initial->post_best_branching();
                full_initial->post_hard_constraints();
                TimeStop ts(5000);
                
                // Fallback: relax problem and find the first feasible solution
                T* relaxed_initial = static_cast<T*>(root->clone());
                relaxed_initial->post_best_branching();
                
                // Options
                Search::Options myopt(opt);
                myopt.clone = true;
                myopt.threads = threads;
                
                // Track whether we have already tried solving the full problem
                bool first = true;
                    
                T* initial = NULL;
                DFS<T>* dfs = NULL;
                
                // Until we find a solution (either feasible or not)
                while(initial == NULL)
                {
                    if (dfs != NULL)
                        delete dfs;
                    dfs = NULL;

                    // First try finding complete solution
                    if (first)
                    {
                        myopt.stop = &ts;
                        dfs = new DFS<T>(full_initial, myopt);
                        first = false;
                    }
                    
                    // Then try solving the relaxed version (until time's up)
                    else
                    {
                        myopt.stop = NULL;
                        dfs = new DFS<T>(relaxed_initial, myopt);
                    }
                    
                    // Find solution
                    initial = dfs->next();
                    
                    // Update statistics    
                    stats += dfs->statistics();
                }
                
                // Delete temporary initial solutions 
                if (full_initial != NULL)
                    delete full_initial;
                full_initial = NULL;
                
                if (relaxed_initial != NULL) 
                    delete relaxed_initial;
                relaxed_initial = NULL;

                // Delete engine used to obtain subsolutions
                if (dfs != NULL)
                    delete dfs;
                dfs = NULL;

                // Initial *must* be non NULL here, so it's safe to assign current
                cur = initial;
                initial = NULL;
                
                // Vector of starting free variables 
                vector<double> starting(ceil(max_free_vars * cur->relaxable_vars()), 0);

                // Set best (if we have a previous best which is worse than current or we don't have a best)                
                if (best == NULL || (cur->cost().val() < best->cost().val()))
                {
                    if (best != NULL)
                        delete best;
                    best = static_cast<T*>(cur->clone());
                }
                
                // Initialize tentative solution
                T* tentative = NULL;
                idle_iterations = 0;

                // Accepted solution
                unsigned int accepted = 0;
                
                // Initialize free variables
                unsigned int cur_free_vars = free_vars;
                unsigned int iter  = 0; 
                
                while (true)
                {
                    // Create new tentative solution
                    tentative = static_cast<T*>(root->clone());
                 
                    // Assign it a fraction of the cur best, also post branching according to "relax" definition
                    int actual_free_vars = cur->relax(tentative, cur_free_vars);
                

                    /** PSEUDO SA:
                        + compute delta that we're willing to accept ad this temperature
                        + constrain solution using delta
                        + always accept solutions
                    */

                    int delta = 0;
                    if (!cur->violations())
                        delta = std::ceil(-(temperature * std::log(p)));

                    // Constrain cost (again, depending on how the constrain method is implemented)
                    if (constrain)
                        tentative->constrain(*cur, delta);
                    
                    // Post random branching (we need randomness to explore) 
                    tentative->post_random_branching();

                    // Set search options
                    Search::Options myopt(opt);
                    myopt.clone = true;
                    myopt.threads = threads;
                    
                    // Allocate a timeout proportional to the number of free vars
                    int timeout = (actual_free_vars * ms_per_var);
                    TimeStop ts(timeout);
                    myopt.stop = &ts;
                    
                    // Run branch and bound on the rest
                    BAB<T>* bab = new BAB<T>(tentative, myopt);

                    if (tentative != NULL)
                        delete tentative;
                    tentative = NULL;
                    
                    // Run branch and bound untile the timeout is over
                    T* prev_tentative = NULL;
                    while ((tentative = bab->next()))
                    {
                        // Pointer to tentative is not nullified because we need it
                        if (prev_tentative != NULL)
                            delete prev_tentative;

                        prev_tentative = tentative;
                    }

                    tentative = prev_tentative;
                    prev_tentative = NULL;

                    // Update stats
                    stats += bab->statistics();
                    delete bab;
                    
                    // Track improvement (simplifies conditions)
                    bool improved;
                    
                    // We haven't found a solution given the current set of constraints and freed variables
                    if (tentative == NULL)
                    {
                        improved = false;
                    }
                    // We have found a solution (it could be better or equal)
                    else
                    {

                        if (!cur->violations())
                            accepted++;

                        if (accepted >= neighbors_accepted)
                        {
                            temperature = std::max(min_temperature, temperature * cooling_rate);
                            accepted = 0;
                        }


                        // Again, keeping previous costs simplifies conditions
                        double best_val, cur_val, tentative_val;
                        improved = false;
                        
                        // If we haven't got rid of violations yet
                        if (tentative->violations())
                        {
                            // Compare on violations
                            best_val = best->violations();
                            cur_val = cur->violations();
                            tentative_val = tentative->violations();
                        }
                        else
                        {
                            // Compare on cost
                            best_val = best->cost().val();
                            cur_val = cur->cost().val();
                            tentative_val = tentative->cost().val();
                        }


                        // If we have improved on the current solution (also allow "lateral" moves)
                        if (tentative_val <= cur_val + delta) // equivalently if (true) 
                        {
                            // We don't set cur to NULL after deletion since it's immediately reset (tentative is not NULL) 
                            if (cur != NULL)
                                delete cur;
                            
                            cur = tentative;

                            // Only mark improvement if we really improve
                            if (tentative_val < cur_val)
                                improved = true;

                            // If we also improve over the best
                            if (tentative_val < best_val)
                            {
#ifdef PRINT_INTERMEDIATE
                                cur->print(cerr);
#endif
                                if (best != NULL)
                                {
                                    delete best;
                                }

                                best = static_cast<T*>(cur->clone());
                            }
                        } 
                        else
                        {
                            delete tentative;
                        }
                        
                        tentative = NULL;
                    }
                    
                    // After comparisons, take measures 
                    if (!improved)
                    {                        
                        // Increase idle iterations
                        idle_iterations++;
                        unsigned int int_max_free_vars = (unsigned int) std::max(ceil(max_free_vars * cur->relaxable_vars()), (double) free_vars);
                                                
                        // Increase variable count (if it's the case)
                        if (idle_iterations >= cur_free_vars * max_idle_iterations)
                        {
                            idle_iterations = 0;
                            
                            // If we're still below the free vars limit, move on to the next number of vars
                            if (cur_free_vars < int_max_free_vars)
                            {
                                // Penalize current number of vars
                                if (distance(starting.begin(), max_element(starting.begin(), starting.end())) != cur_free_vars)
                                    starting[cur_free_vars]--;
                                
                                cur_free_vars++;
                            }
                            else
                            {
                                
                                // Stagnation: tried every possible number of variables and didn't work
                                cur_free_vars = free_vars;
                               
                                // Set probability of random relaxation to 100%
                                double old_rr = random_relaxation;
                                random_relaxation = 1.0;
                                 
                                tentative = NULL;

                                // This shouldn't be necessary
                                while (tentative == NULL)
                                {
                                    // Implement perturbation
                                    tentative = static_cast<T*>(root->clone());

                                    // Relax about double of the max free vars (can be a parameter later)
                                    cur->relax(tentative, 2 * max_free_vars * cur->relaxable_vars());
                                    tentative->post_random_branching();
                                    
                                    // Set search options (no timeout)
                                    Search::Options myopt(opt);
                                    myopt.clone = true;
                                    myopt.threads = threads;
                                
                                    // Run branch and bound on the rest
                                    BAB<T> bab(tentative, myopt);
                                    delete tentative;
                                    
                                    tentative = bab.next();
                                }
 
                                // No need to set cur to null as it's immediately replaced
                                if (cur != NULL)
                                    delete cur;
                                
                                // Set current to perturbated version
                                cur = tentative;
                                tentative = NULL;

#ifdef PRINT_INTERMEDIATE
                                cur->print(cerr);
#endif

                                // Reset original random relaxation
                                random_relaxation = old_rr;
                                
                            }
                        }
                    }
                    
                    // We have found an improving solution
                    else
                    {
                        idle_iterations = 0;

                        // Promote current number of vars 
                        if (distance(starting.begin(), max_element(starting.begin(), starting.end())) != cur_free_vars)
                            starting[cur_free_vars]++;
                        
                        cur_free_vars = (unsigned int) distance(starting.begin(), max_element(starting.begin(), starting.end()));
                        cur_free_vars = std::max(free_vars, cur_free_vars);

                    }

                    // Timeout	
                    if (stop(opt, 0))
                    {
                        if (cur != NULL)
                            delete cur;
                        cur = NULL;
 
                        Space* r = (best != NULL) ? best->clone() : NULL;
                        return r;
                    }
                    
                }
                
                // Final  
                if (cur != NULL)
                    delete cur;
                cur = NULL;

                Space* r = (best != NULL) ? best->clone() : NULL;
                return r;
            }
            
            template <class T>
            forceinline
            LNS<T>::~LNS(void) {
                delete root;
                if (best != NULL)
                    delete best;
            }
        }
    }
}

/** Wrapper for search engine (for scripts) */
namespace Gecode
{
    
    template<class T>
    class LNS : public EngineBase
    {
    private:
        
        /** Real search engine. */
        Search::Engine* e;
        
    public:
        
        LNS(Space* s, const Search::Options& o=Search::Options::def);
        T* next(void);
        Search::Statistics statistics(void) const;
        bool stopped(void) const;
        ~LNS(void);
        
    };
    
    /** Function to initialize search engine (single thread only) */
    namespace Search {
        
        template <class T>
        GECODE_SEARCH_EXPORT Engine* lns(T* s, size_t sz, const Options& o);
        
        template <class T>
        Engine*
        lns(T* s, size_t sz, const Options& o)
        {
#ifdef GECODE_HAS_THREADS
            Options to = o.expand();
            if (to.threads == 1.0)
                return new WorkerToEngine<Sequential::LNS<T> >(s,sz,to);
            else
                return new WorkerToEngine<Sequential::LNS<T> >(s,sz,to);
#else
            return new WorkerToEngine<Sequential::LNS<T> >(s,sz,o);
#endif
            GECODE_NEVER;
            return NULL;
        }
    }
    
    /** Constructor, initializes a search engine of type Sequential::LNS through the exported function Search::LNS */
    template<class T>
    forceinline
    LNS<T>::LNS(Space* s, const Search::Options& o) : e(Search::lns<T>(static_cast<T*>(s),sizeof(T),o)) { }
    
    /** Proxy for next */
    template<class T>
    forceinline T*
    LNS<T>::next(void)
    {
        return static_cast<T*>(e->next());
    }
    
    /** Proxy for statistics */
    template<class T>
    forceinline Search::Statistics
    LNS<T>::statistics(void) const
    {
        return e->statistics();
    }
    
    /** Proxy for stopped */
    template<class T>
    forceinline bool
    LNS<T>::stopped(void) const
    {
        return e->stopped();
    }
    
    /** Destructor */
    template<class T>
    forceinline
    LNS<T>::~LNS(void)
    {
        delete e;
    }
}

// Support for Gist (not working)
namespace Gecode
{
    namespace Driver {
        
        template<typename S>
        class GistEngine<LNS<S> > {
        public:
            static void explore(S* root, const Gist::Options& opt)
            {
                (void) Gist::bab(root, opt);
            }
        };
    }
}

template <class T>
unsigned int Gecode::Search::Sequential::LNS<T>::max_idle_iterations = 100;

template <class T>
unsigned int Gecode::Search::Sequential::LNS<T>::free_vars = 1;

template <class T>
double Gecode::Search::Sequential::LNS<T>::max_free_vars = 0.15;

template <class T>
unsigned int Gecode::Search::Sequential::LNS<T>::threads = 1;

template <class T>
bool Gecode::Search::Sequential::LNS<T>::constrain = true;

template <class T>
unsigned int Gecode::Search::Sequential::LNS<T>::ms_per_var = 10;

template <class T>
double Gecode::Search::Sequential::LNS<T>::random_relaxation = 0.0;

template <class T>
bool Gecode::Search::Sequential::LNS<T>::random = false;

template <class T>
double Gecode::Search::Sequential::LNS<T>::temperature = 20;

template <class T>
double Gecode::Search::Sequential::LNS<T>::min_temperature = 0.4;

template <class T>
double Gecode::Search::Sequential::LNS<T>::cooling_rate = 0.98;

template <class T>
double Gecode::Search::Sequential::LNS<T>::p = 0.05;

template <class T>
int Gecode::Search::Sequential::LNS<T>::neighbors_accepted = 20;

#endif
