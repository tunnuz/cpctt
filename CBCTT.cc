#include "CBCTT.hh"

void CBCTT::post_best_branching()
{
    // Post branching rules 
    branch(*this, roomslot, INT_VAR_DEGREE_MAX(), INT_VAL_MIN());
}



void CBCTT::post_random_branching()
{
    // Post branching rules
    branch(*this, roomslot, INT_VAR_RND(0), INT_VAL_RND(0));
}


Faculty CBCTT::in;
