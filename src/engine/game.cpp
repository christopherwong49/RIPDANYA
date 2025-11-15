#include "game.hpp"

void Game::make_move(Move move) {
	hist.push_back(cur_pos);
	cur_pos.make_move(move);
}

void Game::unmake_move() {
	cur_pos = hist.back();
	hist.pop_back();
}

void Game::commit() {
	hist.clear();
}
