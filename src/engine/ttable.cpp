#include "ttable.hpp"

void TTable::clear() {
	memset(data, 0, 268435440);
}

TTEntry *TTable::probe(uint64_t key) {
	TTEntry &entry = data[key % TTSIZE];
	if (entry.key == key)
		return &entry;

	return nullptr;
}

void TTable::store(uint64_t key, Move move, int depth, Value score, TTFlag flag) {
	TTEntry &entry = data[key % TTSIZE];
	entry.key = key;
	entry.move = move;
	entry.depth = depth;
	entry.score = score;
	entry.flag = flag;
}
