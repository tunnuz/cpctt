#ifndef CP_CTT_random_hh
#define CP_CTT_random_hh

#include <random>

/** A class to generate random numbers with Mersenne Twister */
class Random
{
public:
    
    /** Mersenne Twister engine */
    static std::mt19937 g;
    
    /** Randomness generating device */
    static std::random_device dev;
    
    
    /** Generates an uniform random integer in [a,b].
     @param a lower bound
     @param b upper bound
     */
    static unsigned int Int(unsigned int a, unsigned int b)
    {
        std::uniform_int_distribution<> d(a,b);
        return d(g);
    }
    
    /** Generates an uniform random double in [a,b]
     @param a lower bound
     @param b upper bound
     @remarks generates an uniform random double in [0,1] if called without arguments
     */
    static double Double(double a = 0, double b = 1)
    {
        std::uniform_real_distribution<> d(a,b);
        return d(g);
    }
    
    /** Sets a new seed for the random engine. */
    static void Seed(unsigned int seed)
    {
        g.seed(seed);
        Random::seed = seed;
    }
    
    /** Seed */
    static unsigned int seed;
};

class State;

class GenerationDependent
{
public:
    
    virtual void generation_started(State& q, unsigned int g) { }
};



#endif
