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

	std::stack<HistoryEntry> move_hist;

	Position() {}

	Position(std::string fen) {
		load_fen(fen);
	}

	// void set_piece(std::string);

	void load_fen(std::string);

	void make_move(const Move);

	void unmake_move();

	void legal_moves(rip::vector<Move> &) const;
	bool control(int, bool) const; // control(square, side): is square controlled by side

	void recompute_hash();

	void print_board() const;
};

struct HistoryEntry {
	Move move;
	Piece prev_piece;
	uint8_t prev_cas;
	uint8_t prev_ep;
	uint8_t halfmove;

	HistoryEntry(Move move, Piece prev_piece, u_int8_t prev_cas, u_int8_t prev_ep, uint8_t halfmove){
		move = move;
		prev_piece = prev_piece;
		prev_cas = prev_cas;
		prev_ep = prev_ep;
		halfmove = halfmove;
	}
};
