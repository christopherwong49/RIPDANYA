#pragma once

#include "bitboard.hpp"
#include "defines.hpp"
#include "move.hpp"

#include <string>

#define OCC(side) (6 ^ (side))
#define OPPOCC(side) (7 ^ (side))

constexpr uint8_t SQ_NONE = 64;

class Move;

struct Position {
	Bitboard piece_boards[8]; // 6 pieces + white & black
	Piece mailbox[64];

	bool side;
	uint8_t halfmove;
	uint8_t castling;
	uint8_t ep_square;
	uint64_t zobrist;

	Position() {}

	Position(std::string fen) {
		load_fen(fen);
		recompute_hash();
	}

	// void set_piece(std::string);

	void load_fen(std::string);

	void make_move(const Move);

	void legal_moves(rip::vector<Move> &) const;
	bool control(int, bool) const; // control(square, side): is square controlled by side

	void recompute_hash();

	void print_board() const;

	void checkhash(uint64_t);
};
