#include "game.hpp"

void Game::make_move(Move move) {
	hist.push_back(cur_pos);
	cur_pos.make_move(move);
	hash_hist.push_back(cur_pos.zobrist);
}

void Game::unmake_move() {
	cur_pos = hist.back();
	hist.pop_back();
	hash_hist.pop_back();
}

void Game::commit() {
	hist.clear();
}

void Game::clear_hist() {
	hist.clear();
	hash_hist.clear();
}

bool Game::threefold() const {
	if (hash_hist.empty())
		return false;
	int count = 0;
	for (int i = hash_hist.size() - 1; i >= 0; i--) {
		if (hash_hist[i] == cur_pos.zobrist) {
			count++;
			if (count >= 3)
				return true;
		}
	}
	return false;
}
