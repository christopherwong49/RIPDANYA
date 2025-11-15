#pragma once

#include "defines.hpp"
#include "position.hpp"

Value eval(Position &p);

struct Accumulator {
	int16_t val[1024] = {0};
};

void accumulator_add(Accumulator &acc, int index);
void accumulator_sub(Accumulator &acc, int index);
