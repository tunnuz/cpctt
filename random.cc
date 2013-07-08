//
//  random.cc
//  CP-CTT
//
//  Created by Tommaso Urli on 6/17/13.
//  Copyright (c) 2013 Tommaso Urli. All rights reserved.
//

#include "random.hh"

using namespace std;

std::random_device Random::dev;

std::mt19937 Random::g(Random::dev());

unsigned int Random::seed;