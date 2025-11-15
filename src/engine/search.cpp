#include "search.hpp"

uint64_t perft(Game &g, int depth) {
	Position &p = g.pos();

	if (p.side == WHITE && p.control(_tzcnt_u64(p.piece_boards[KING] & p.piece_boards[OCC(BLACK)]), WHITE))
		return 0;
	if (p.side == BLACK && p.control(_tzcnt_u64(p.piece_boards[KING] & p.piece_boards[OCC(WHITE)]), BLACK))
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
bool early_exit = false;
bool can_exit = false;

clock_t start_time;
// clock_t end_time;
uint64_t mx_time;

Value qsearch(Game &g, Value alpha, Value beta) {
	nodes++;

	if ((nodes & 0xfff) == 0) {
		if (can_exit && (clock() - start_time) / (CLOCKS_PER_SEC / 1000) >= mx_time) {
			early_exit = true;
			return 0;
		}
	}

	Position &board = g.pos();
	if (board.control(_tzcnt_u64(board.piece_boards[5] & board.piece_boards[OCC(!board.side)]), board.side)) // checkmate, we won
		return VALUE_MATE;

	rip::vector<Move> moves;
	board.legal_moves(moves);

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

			if (early_exit)
				return 0;

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

	if ((nodes & 0xfff) == 0) {
		if (can_exit && (clock() - start_time) / (CLOCKS_PER_SEC / 1000) >= mx_time) {
			early_exit = true;
			return 0;
		}
	}

	Position &board = g.pos();

	bool in_check = board.control(_tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(board.side)]), !board.side);
	if (board.control(_tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(!board.side)]), board.side)) // checkmate, we won
		return VALUE_MATE;

	if (depth <= 0)
		return qsearch(g, alpha, beta);

	rip::vector<Move> moves;
	board.legal_moves(moves);

	// order_moves(board, moves, ply);

	Value best = -VALUE_INFINITE;
	Move best_move = NullMove;

	for (Move &mv : moves) {
		g.make_move(mv);
		Value score = -negamax(g, depth - 1, ply + 1, -beta, -alpha, false);
		g.unmake_move();
		if (early_exit)
			return 0;

		if (score > best) {
			best = score;
			if (score > alpha) {
				alpha = score;
				best_move = mv;
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

	if (best == -VALUE_MATE) {
		if (in_check)
			return -VALUE_MATE + ply;
		else
			return 0;
	}

	if (root)
		g_best = best_move;
	return best;
}

std::string score_to_string(Value score) {
	if (score >= VALUE_MATE_MAX_PLY) {
		return "mate " + std::to_string((VALUE_MATE - score + 1) / 2);
	} else if (score <= -VALUE_MATE_MAX_PLY) {
		return "mate " + std::to_string((-VALUE_MATE - score) / 2);
	} else {
		return "cp " + std::to_string(score);
	}
}

Move search(Game &g, int time, int depth) {
	start_time = clock();
	mx_time = time;
	nodes = 0;
	can_exit = false;
	early_exit = false;
	g_best = NullMove;

	for (int i = 1; i <= depth; i++) {
		Value score = negamax(g, i, 0, -VALUE_INFINITE, VALUE_INFINITE, true);
		if (early_exit)
			break;
		std::cout << "info depth " << i << " score " << score_to_string(score) << " nodes " << nodes << " pv " << g_best.to_string() << std::endl;
		can_exit = true;
	}

	return g_best;
}
