#pragma once

#include "game.hpp"
#include "move.hpp"
#include "nnue.hpp"

struct SSEntry {
	Move killer0, killer1;
};

extern uint64_t nodes;

uint64_t perft(Game &g, int depth);

Move search(Game &g, int time, int depth);
