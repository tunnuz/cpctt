#include <iostream>
#include <fstream>
#include "CBCTT.hh"
#include "LNS.hh"

using namespace std;

int main(int argc, char * argv[])
{
    // Read options
    InstanceOptions opt("");
    opt.model(0, "debug", "debug model (print lots of stuff)");
    opt.model(1, "experiments", "silent model only prints a solution in the end"); // default
    opt.model(1);
    
    opt.parse(argc, argv);
    
    // Open .cfg file
    fstream fs(opt.instance());
    
    typedef Gecode::Search::Sequential::LNS<LNSCBCTT> MyLNS;
    
    string selector, instance, random;
    while (fs >> selector)
    {
        if (selector == "instance")
            fs >> instance;
        if (selector == "init-free-variables")
            fs >> MyLNS::free_vars;
        if (selector == "max-free-variables")
            fs >> MyLNS::max_free_vars;
        if (selector == "ms-per-variable")
            fs >> MyLNS::ms_per_var;
        if (selector == "max-idle-iterations")
            fs >> MyLNS::max_idle_iterations;
        if (selector == "random-branching")
            fs >> random;
        if (selector == "random-relaxation")
            fs >> MyLNS::random_relaxation;
        if (selector == "temperature")
            fs >> MyLNS::temperature;
        if (selector == "neighbors-accepted")
            fs >> MyLNS::temperature;
        if (selector == "delta-probability")
            fs >> MyLNS::p;
        if (selector == "min-temperature")
            fs >> MyLNS::min_temperature;
        if (selector == "cooling-rate")
            fs >> MyLNS::cooling_rate;
    }
    
    fs.close();
    opt.instance(instance.c_str());
    
    try
    {
        Script::run<LNSCBCTT, LNS, InstanceOptions>(opt);
        //Script::run<InstantBranchingSpace<CBCTT>, BAB, InstanceOptions>(opt);
    }
    catch(std::exception e)
    {
        std::cout << e.what() << std::endl;
    }
    return 0;
}

