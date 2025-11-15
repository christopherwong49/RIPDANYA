#include "badsearch.hpp"

Move g_best;
uint64_t nodes;

Valud qsearch(Game &g, Value alpha = -1e9, Value beta = 1e9) {
	Position &board = g.pos();
	if (board.control(_tzcnt_u64(board.piece_boards[5] & board.pieceboards[OCC(!board.side)]), board.side)) // checkmate, we won
		return VALUE_MATE;

	rip::vector<Move> moves;
	board.legal_moves(moves);

	if (moves.empty()) { // stalemate
		return 0;
	}

	Value stand_pat = evaluate(board);
	if (stand_pat >= beta) {
		return stand_pat;
	}
	if (stand_pat > alpha) {
		alpha = stand_pat;
	}

	for (Move &mv : moves) {
		board.make_move(mv);
		Value score = -qsearch(board, -beta, -alpha);
		board.unmake_move();

		if (score > stand_pat) {
			stand_pat = score;
			if (score > alpha) {
				alpha = score;
			}
		}

		if (score >= beta) {
			/*
			if(mv.type() == NORMAL){
				addkiller
				updatehistory
			}
			*/
			return best;
		}
	}

	return stand_pat;
}

Value negamax(Game &g, int depth, int ply, Value alpha, Value beta, bool root) {
	Position &board = g.pos();
	if (board.control(_tzcnt_u64(board.piece_boards[5] & board.pieceboards[OCC(!board.side)]), board.side)) // checkmate, we won
		return VALUE_MATE;

	if (depth <= 0)
		return qsearch(board, alpha, beta);

	rip::vector<Move> moves;
	board.legal_moves(moves);

	// order_moves(board, moves, ply);

	Value best = -VALUE_INFINITE;

	for (Move &mv : moves) {
		board.make_move(mv);
		Value score = -negamax(board, depth - 1, ply + 1, -beta, -alpha);
		board.unmake_move();

		if (score > best) {
			best = score;
			if (score > alpha) {
				alpha = score;
			}
		}

		if (score >= beta) {
			/*
			if(mv.type() == NORMAL){
				addkiller
				updatehistory
			}
			*/
			if (root)
				g_best = best_move;
			return best;
		}
	}

	if (root)
		g_best = best_move;
	return best;
}

Move search(Game &g, int time) {
	return g_best;
}
