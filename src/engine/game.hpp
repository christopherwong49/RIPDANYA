#pragma once

#include "position.hpp"

class Game {
private:
	rip::vector<Position> hist;
	Position cur_pos;

public:
	Game(std::string fen) : cur_pos(fen) {}

	void make_move(Move m);
	void unmake_move();
	void commit();

	constexpr Position &pos() {
		return cur_pos;
	}
};
