#pragma once

#include "defines.hpp"
#include "move.hpp"

#define TTSIZE (268435456 / sizeof(TTEntry))

enum TTFlag { NONE, UPPER_BOUND, LOWER_BOUND, EXACT };

struct TTEntry {
	uint64_t key;
	Move move;
	uint16_t depth;
	Value score, s_eval;
	TTFlag flag;
};

class TTable {
private:
	TTEntry *data;

public:
	TTable() {
		data = new TTEntry[TTSIZE];
		clear();
	}
	~TTable() {
		delete[] data;
	}
	void operator=(const TTable &other) = delete;

	void clear();
	TTEntry *probe(uint64_t key);
	void store(uint64_t key, Move move, int depth, Value score, Value s_eval, TTFlag flag);

	static constexpr Value mate_from_tt(Value ttscore, int ply) {
		if (ttscore <= -VALUE_MATE_MAX_PLY)
			return ttscore + ply;
		if (ttscore >= VALUE_MATE_MAX_PLY)
			return ttscore - ply;
		return ttscore;
	}

	static constexpr Value mate_to_tt(Value score, int ply) {
		if (score <= -VALUE_MATE_MAX_PLY)
			return score - ply;
		if (score >= VALUE_MATE_MAX_PLY)
			return score + ply;
		return score;
	}
};
