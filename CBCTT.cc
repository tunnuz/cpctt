#include "CBCTT.hh"

void CBCTT::neighborhood_branching()
{
    // Post branching rules 
    branch(*this, roomslot, INT_VAR_DEGREE_MAX(), INT_VAL_MIN());
}



void CBCTT::initial_solution_branching(unsigned long int restarts)
{
    // Post branching rules
    branch(*this, roomslot, INT_VAR_RND(restarts), INT_VAL_RND(restarts));
}


Faculty CBCTT::in;
