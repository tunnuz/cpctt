#include <iostream>
#include <fstream>
#include "CBCTT.hh"
#include "gecode-lns/lns.h"

using namespace std;

template <typename>
class LNSCBCTT_ME : public LNS<BAB, LNSCBCTT>
{
public:
  LNSCBCTT_ME(LNSCBCTT* s, const Search::Options& o) : LNS<BAB, LNSCBCTT>(s, o) {}
};

int main(int argc, char * argv[])
{
    // Read options
    LNSInstanceOptions opt("");
    opt.model(0, "debug", "debug model (print lots of stuff)");
    opt.model(1, "experiments", "silent model only prints a solution in the end"); // default
    opt.model(0);
    
    opt.parse(argc, argv);
  
  
    // The lns options (FIXME: to remove)
    Gecode::Search::Meta::LNS::lns_options = &opt;

    try
    {
        cerr << opt.minIntensity() << endl;
      
        Script::run<LNSCBCTT, LNSCBCTT_ME, LNSInstanceOptions>(opt);
        //Script::run<InstantBranchingSpace<CBCTT>, BAB, InstanceOptions>(opt);
    }
    catch(std::exception e)
    {
        std::cout << e.what() << std::endl;
    }
    return 0;
}

