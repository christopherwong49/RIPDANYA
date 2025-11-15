#include "search.hpp"

uint64_t perft(Game &g, int depth) {
	Position &p = g.pos();

	if (p.side == WHITE && p.control(__tzcnt_u64(p.piece_boards[KING] & p.piece_boards[OCC(BLACK)]), WHITE))
		return 0;
	if (p.side == BLACK && p.control(__tzcnt_u64(p.piece_boards[KING] & p.piece_boards[OCC(WHITE)]), BLACK))
		return 0;

	if (depth == 0)
		return 1;

	rip::vector<Move> moves;
	p.legal_moves(moves);

	uint64_t nodes = 0;
	for (size_t i = 0; i < moves.size(); i++) {
		g.make_move(moves[i]);
		nodes += perft(g, depth - 1);
		g.unmake_move();
	}

	return nodes;
}

Move g_best;
uint64_t nodes;

Value qsearch(Game &g, Value alpha = -1e9, Value beta = 1e9) {
	nodes++;

	Position &board = g.pos();
	if (board.control(_tzcnt_u64(board.piece_boards[5] & board.piece_boards[OCC(!board.side)]), board.side)) // checkmate, we won
		return VALUE_MATE;

	rip::vector<Move> moves;
	board.legal_moves(moves);

	if (moves.empty()) { // stalemate
		return 0;
	}

	Value stand_pat = eval(board);
	if (stand_pat >= beta) {
		return stand_pat;
	}
	if (stand_pat > alpha) {
		alpha = stand_pat;
	}

	for (Move &mv : moves) {
		if (board.mailbox[mv.dst()] != NO_PIECE) { // capture
			g.make_move(mv);
			Value score = -qsearch(g, -beta, -alpha);
			g.unmake_move();

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
				return stand_pat;
			}
		}
	}

	return stand_pat;
}

Value negamax(Game &g, int depth, int ply, Value alpha, Value beta, bool root) {
	nodes++;

	Position &board = g.pos();
	if (board.control(_tzcnt_u64(board.piece_boards[5] & board.piece_boards[OCC(!board.side)]), board.side)) // checkmate, we won
		return VALUE_MATE;

	if (depth <= 0)
		return qsearch(g, alpha, beta);

	rip::vector<Move> moves;
	board.legal_moves(moves);

	// order_moves(board, moves, ply);

	Value best = -VALUE_INFINITE;

	for (Move &mv : moves) {
		g.make_move(mv);
		Value score = -negamax(g, depth - 1, ply + 1, -beta, -alpha, false);
		g.unmake_move();

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
				g_best = best;
			return best;
		}
	}

	if (root)
		g_best = best;
	return best;
}

Move search(Game &g, int time) {
	return g_best;
}
