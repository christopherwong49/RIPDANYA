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
int capthist[6][6][64];
int corrhist[2][32768];

void update_history(bool side, Move m, int bonus) {
	bonus = std::clamp(bonus, -16384, 16384);
	int &val = history[side][m.src()][m.dst()];
	val += bonus - val * std::abs(bonus) / 16384;
}

void update_capthist(PieceType atk, PieceType victim, int dst, int bonus) {
	bonus = std::clamp(bonus, -16384, 16384);
	int &val = capthist[atk][victim][dst];
	val += bonus - val * std::abs(bonus) / 16384;
}

void update_corrhist(Position &pos, Value diff, int depth) {
	const int sdiff = diff * 256;
	const int weight = std::min(16, 1 + depth);

	auto update_entry = [&](int &entry) {
		int update = entry * (256 - weight) + sdiff * weight;
		entry = std::clamp(update / 256, -16384, 16384);
	};

	update_entry(corrhist[pos.side][pos.pawn_hash & 0x7fff]);
}

void apply_correction(Position &pos, Value &eval) {
	int corr = 0;
	corr += corrhist[pos.side][pos.pawn_hash & 0x7fff];

	eval += corr / 256;
}

SSEntry ss[MAX_PLY];

int LMR[256][MAX_PLY];
__attribute__((constructor)) void init_lmr() {
	for (int i = 1; i < 256; i++) {
		for (int d = 1; d < MAX_PLY; d++) {
			LMR[i][d] = 0.77 + log(i) * log(d) / 2.36;
		}
	}
}

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
	apply_correction(board, stand_pat);
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
	if (!pv && ent && ent->depth >= d) {
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

	if (in_check)
		d += params::CHECK_EXT;

	if (d <= 0)
		return qsearch(g, alpha, beta);

	Value raw_eval = eval(board);
	Value cur_eval = raw_eval;
	apply_correction(board, cur_eval);

	// RFP
	if (!pv && !in_check && d <= 8 && cur_eval - params::RFP_MARGIN * d >= beta)
		return cur_eval;

	// NMP
	if (!in_check && cur_eval >= beta && (board.piece_boards[KNIGHT] | board.piece_boards[BISHOP] | board.piece_boards[ROOK] | board.piece_boards[QUEEN])) {
		int r = params::NMP_R + d / 4 + std::min((cur_eval - beta) / 400, 3);

		g.make_move(NullMove);
		Value score = -negamax(g, d - r, ply + 1, -beta, -beta + 1, false, false);
		g.unmake_move();
		if (early_exit)
			return 0;

		if (score >= beta)
			return score;
	}

	// Razoring
	if (!pv && !in_check && d <= 8 && cur_eval + params::RAZOR_MARGIN * d < alpha) {
		Value score = qsearch(g, alpha, alpha + 1);
		if (score <= alpha)
			return score;
	}

	rip::vector<Move> moves;
	rip::vector<std::pair<int, Move>> order;
	board.legal_moves(moves);

	// Internal iterative reduction
	if (pv && (!ent || ent->move == NullMove) && d >= 6) {
		d--;
	}

	// Move ordering
	for (Move &m : moves) {
		constexpr int TT_BASE = 10000000;
		constexpr int CAPT_BASE = 1000000;
		constexpr int KILL_BASE = 100000;

		int score = 0;
		if (ent && m == ent->move) {
			score += TT_BASE;
		} else if (board.mailbox[m.dst()] != NO_PIECE) {
			// Captures
			score += PieceValues[board.mailbox[m.dst()] & 7] + capthist[board.mailbox[m.src()] & 7][board.mailbox[m.dst()] & 7][m.dst()] + CAPT_BASE;
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

	rip::vector<Move> quiets, captures;

	for (auto &[_, mv] : order) {
		bool capt = board.mailbox[mv.dst()] != NO_PIECE;

		g.make_move(mv);

		Value score;

		if (d >= 2 && i >= 2 + 2 * pv) {
			int r = LMR[i][d];
			int newdepth = d - 1;
			int lmrdepth = newdepth - r;

			score = -negamax(g, lmrdepth, ply + 1, -alpha - 1, -alpha, false, false);
			if (score > alpha && lmrdepth < newdepth) {
				score = -negamax(g, newdepth, ply + 1, -alpha - 1, -alpha, false, false);
			}
		} else if (!pv || i > 1) {
			// Standard null window
			score = -negamax(g, d - 1, ply + 1, -alpha - 1, -alpha, false, false);
		}
		if (pv && (i == 1 || score > alpha)) {
			// Full depth + window
			score = -negamax(g, d - 1, ply + 1, -beta, -alpha, false, true);
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
			const int bonus = 4 * d * d;
			if (!capt && mv.type() != PROMOTION) {
				update_history(board.side, mv, bonus);
				for (Move &qm : quiets) {
					update_history(board.side, qm, -bonus);
				}

				if (ss[ply].killer0 != mv && ss[ply].killer1 != mv) {
					ss[ply].killer1 = ss[ply].killer0;
					ss[ply].killer0 = mv;
				}
			} else if (capt) {
				PieceType atk = PieceType(board.mailbox[mv.src()] & 7);
				PieceType victim = PieceType(board.mailbox[mv.dst()] & 7);
				update_capthist(atk, victim, mv.dst(), bonus);
			}
			for (Move &cm : captures) {
				PieceType atk = PieceType(board.mailbox[cm.src()] & 7);
				PieceType victim = PieceType(board.mailbox[cm.dst()] & 7);
				update_capthist(atk, victim, cm.dst(), -bonus);
			}

			if (root)
				g_best = best_move;

			if (!in_check && mv.type() != PROMOTION && !capt && best > raw_eval) {
				update_corrhist(board, best - raw_eval, d);
			}

			g.ttable.store(board.zobrist, best_move, d, best, LOWER_BOUND);
			return best;
		}

		i++;

		if (!capt && mv.type() != PROMOTION)
			quiets.push_back(mv);
		else if (capt)
			captures.push_back(mv);
	}

	if (best == -VALUE_MATE) {
		if (in_check)
			return -VALUE_MATE + ply;
		else
			return 0;
	}

	if (root)
		g_best = best_move;

	bool best_iscapture = best_move != NullMove && board.mailbox[best_move.dst()] != NO_PIECE;
	if (!in_check && !(flag == UPPER_BOUND && best >= raw_eval) && !(best_move != NullMove && (best_iscapture || best_move.type() == PROMOTION))) {
		update_corrhist(board, best - raw_eval, d);
	}

	Value ttstore = TTable::mate_to_tt(best, ply);
	Move ttmove = best_move;
	if (ttmove == NullMove && ent)
		ttmove = ent->move;
	g.ttable.store(board.zobrist, ttmove, d, ttstore, flag);
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
