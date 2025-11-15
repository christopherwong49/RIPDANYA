#pragma once

#include "defines.hpp"
#include "position.hpp"

Value eval(Position &p);

struct Accumulator {
	int16_t val[1024];
};

struct EvalState {
	Accumulator w_acc, b_acc;
	Piece mailbox[64];

	EvalState() {}
};

void accumulator_add(Accumulator &acc, int index);
void accumulator_sub(Accumulator &acc, int index);
