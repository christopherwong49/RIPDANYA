#pragma once

#include "position.hpp"

class Game {
private:
	Position cur_pos;
	rip::vector<Position, 512> hist;
	rip::vector<uint64_t, 1024> hash_hist;

public:
	Game(std::string fen) : cur_pos(fen) {}

	void make_move(Move m);
	void unmake_move();
	void commit();
	void clear_hist();
	bool threefold() const;

	constexpr Position &pos() {
		return cur_pos;
	}
};
