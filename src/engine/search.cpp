#include "search.hpp"

#include "nnue.hpp"
#include "params.hpp"
#include "ttable.hpp"

uint64_t perft(Game &g, int d) {
	Position &p = g.pos();

	if (p.side == WHITE && p.control(_tzcnt_u64(p.piece_boards[KING] & p.piece_boards[OCC(BLACK)]), WHITE))
		return 0;
	if (p.side == BLACK && p.control(_tzcnt_u64(p.piece_boards[KING] & p.piece_boards[OCC(WHITE)]), BLACK))
		return 0;

	if (d == 0)
		return 1;

	rip::vector<Move> moves;
	p.legal_moves(moves);

	uint64_t nodes = 0;
	for (size_t i = 0; i < moves.size(); i++) {
		g.make_move(moves[i]);
		nodes += perft(g, d - 1);
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

int MVV_LVA[6][6];
__attribute__((constructor)) void init_mvv_lva() {
	for (int a = 0; a < 6; a++) {
		for (int b = 0; b < 6; b++) {
			MVV_LVA[a][b] = PieceValues[a] * ScaleValue - PieceValues[b];
		}
	}
}

int history[2][64][64];

SSEntry ss[MAX_PLY];

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

	Value stand_pat = eval(board);
	if (stand_pat >= beta) {
		return stand_pat;
	}
	if (stand_pat > alpha) {
		alpha = stand_pat;
	}

	rip::vector<Move> moves;
	rip::vector<std::pair<int, Move>> order;
	board.legal_moves(moves);

	// MVV-LVA ordering
	for (Move &m : moves) {
		if (board.mailbox[m.dst()] != NO_PIECE) {
			int score = MVV_LVA[board.mailbox[m.dst()] & 7][board.mailbox[m.src()] & 7];
			order.push_back({-score, m});
		}
	}
	std::stable_sort(order.begin(), order.end());

	for (auto &[_, m] : order) {
		if (board.mailbox[m.dst()] != NO_PIECE) {
			g.make_move(m);
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

Value negamax(Game &g, int d, int ply, Value alpha, Value beta, bool root, bool pv) {
	nodes++;

	if ((nodes & 0xfff) == 0) {
		if (can_exit && (clock() - start_time) / (CLOCKS_PER_SEC / 1000) >= mx_time) {
			early_exit = true;
			return 0;
		}
	}

	Position &board = g.pos();

	// TT Cutoffs
	TTEntry *ent = g.ttable.probe(board.zobrist);
	if (ent && ent->depth >= d) {
		Value ttscore = TTable::mate_from_tt(ent->score, ply);
		if (ent->flag == EXACT)
			return ttscore;
		if (ent->flag == LOWER_BOUND && ttscore >= beta)
			return ttscore;
		if (ent->flag == UPPER_BOUND && ttscore <= alpha)
			return ttscore;
	}

	bool in_check = board.control(_tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(board.side)]), !board.side);
	if (board.control(_tzcnt_u64(board.piece_boards[KING] & board.piece_boards[OCC(!board.side)]), board.side)) // checkmate, we won
		return VALUE_MATE;

	if (!root && g.threefold())
		return 0;

	if (d <= 0)
		return qsearch(g, alpha, beta);

	Value cur_eval = eval(board);

	// RFP
	if (!pv && !in_check && d <= 8 && cur_eval - params::RFP_MARGIN * d >= beta)
		return cur_eval;

	// NMP
	if (!in_check && (board.piece_boards[KNIGHT] | board.piece_boards[BISHOP] | board.piece_boards[ROOK] | board.piece_boards[QUEEN])) {
		g.make_move(NullMove);
		Value score = -negamax(g, d - params::NMP_R, ply + 1, -beta, -beta + 1, false, false);
		g.unmake_move();
		if (early_exit)
			return 0;

		if (score >= beta)
			return score;
	}

	rip::vector<Move> moves;
	rip::vector<std::pair<int, Move>> order;
	board.legal_moves(moves);

	// Move ordering
	for (Move &m : moves) {
		constexpr int CAPT_BASE = 1000000;
		constexpr int KILL_BASE = 100000;

		int score = 0;
		if (board.mailbox[m.dst()] != NO_PIECE) {
			// Captures
			score += MVV_LVA[board.mailbox[m.dst()] & 7][board.mailbox[m.src()] & 7] + CAPT_BASE;
		} else {
			// History heuristic
			score += history[board.side][m.src()][m.dst()];

			// Killer moves
			if (ss[ply].killer0 == m || ss[ply].killer1 == m)
				score += KILL_BASE;
		}
		order.push_back({-score, m});
	}
	std::stable_sort(order.begin(), order.end());

	Value best = -VALUE_INFINITE;
	Move best_move = NullMove;
	TTFlag flag = UPPER_BOUND;
	int i = 1;
	for (auto &[_, mv] : order) {
		bool capt = board.mailbox[mv.dst()] != NO_PIECE;

		g.make_move(mv);

		Value score;
		if (i == 1) {
			score = -negamax(g, d - 1, ply + 1, -beta, -alpha, false, pv);
		} else {
			score = -negamax(g, d - 1, ply + 1, -alpha - 1, -alpha, false, false);
			if (score > alpha && score < beta) {
				score = -negamax(g, d - 1, ply + 1, -beta, -alpha, false, pv);
			}
		}

		g.unmake_move();
		if (early_exit)
			return 0;

		if (score > best) {
			best = score;
			if (score > alpha) {
				alpha = score;
				best_move = mv;
				flag = EXACT;
			}
		}

		if (score >= beta) {
			if (!capt && mv.type() != PROMOTION) {
				history[board.side][mv.src()][mv.dst()] += d * d;

				if (ss[ply].killer0 != mv && ss[ply].killer1 != mv) {
					ss[ply].killer1 = ss[ply].killer0;
					ss[ply].killer0 = mv;
				}
			}

			if (root)
				g_best = best_move;
			g.ttable.store(board.zobrist, best_move, d, (board.side == WHITE ? best : -best), LOWER_BOUND);
			return best;
		}

		i++;
	}

	if (best == -VALUE_MATE) {
		if (in_check)
			return -VALUE_MATE + ply;
		else
			return 0;
	}

	if (root)
		g_best = best_move;

	Value ttstore = TTable::mate_to_tt(best, ply);
	g.ttable.store(board.zobrist, best_move, d, ttstore, flag);
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

Move search(Game &g, int time, int d) {
	start_time = clock();
	mx_time = time;
	nodes = 0;
	can_exit = false;
	early_exit = false;
	g_best = NullMove;

	memset(history, 0, sizeof(history));
	memset(ss, 0, sizeof(ss));

	for (int i = 1; i <= d; i++) {
		Value score = negamax(g, i, 0, -VALUE_INFINITE, VALUE_INFINITE, true, true);
		if (early_exit)
			break;
		std::cout << "info depth " << i << " score " << score_to_string(score) << " nodes " << nodes << " pv " << g_best.to_string() << std::endl;
		can_exit = true;
	}

	return g_best;
}
