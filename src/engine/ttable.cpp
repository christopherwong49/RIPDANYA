#include "ttable.hpp"

void TTable::clear() {
	memset(data, 0, 268435456);
}

TTEntry *TTable::probe(uint64_t key) {
	for (int i = 0; i < 2; i++) {
		TTEntry *entry = &data[key % TTSIZE].entries[i];
		if (entry->key == key)
			return entry;
	}

	return nullptr;
}

void TTable::store(uint64_t key, Move move, int depth, Value score, TTFlag flag) {
	// TTEntry &entry = data[key % TTSIZE];
	// entry.key = key;
	// entry.move = move;
	// entry.depth = depth;
	// entry.score = score;
	// entry.flag = flag;

	TTEntry &always = data[key % TTSIZE].entries[0];
	TTEntry &d = data[key % TTSIZE].entries[1];

	if (d.key == key || always.key == key) {
		// update entry
		if (d.key == key) {
			d.move = move;
			d.depth = depth;
			d.score = score;
			d.flag = flag;
		} else {
			always.move = move;
			always.depth = depth;
			always.score = score;
			always.flag = flag;
		}
		return;
	}

	// check depth entry
	if (d.depth < depth) {
		// replace depth entry
		d.key = key;
		d.move = move;
		d.depth = depth;
		d.score = score;
		d.flag = flag;
		return;
	}

	// replace always entry
	always.key = key;
	always.move = move;
	always.depth = depth;
	always.score = score;
	always.flag = flag;
}
