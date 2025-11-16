#pragma once

#include "defines.hpp"
#include "game.hpp"
#include "move.hpp"

struct ContHist {
	int hist[2][6][64];
};

struct SSEntry {
	Move killer0, killer1;
	ContHist *cont_hist;
};

extern uint64_t nodes;

uint64_t perft(Game &g, int depth);

Move search(Game &g, int time, int depth);
