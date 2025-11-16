#pragma once

#include "defines.hpp"
#include "game.hpp"
#include "move.hpp"

struct SSEntry {
	Move killer0, killer1;
	Value score;
};

extern uint64_t nodes;

uint64_t perft(Game &g, int depth);

Move search(Game &g, int time, int depth);
