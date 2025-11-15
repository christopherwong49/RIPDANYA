#include "bitboard.hpp"
#include "move.hpp"
#include "position.hpp"

#include <iostream>

Bitboard knight_movetable[64];
Bitboard king_movetable[64];
Bitboard rook_movetable[64][1 << 14];
Bitboard bishop_movetable[64][1 << 13];

Bitboard rook_masks[64];
Bitboard bishop_masks[64];

void gen_rook_moves(int sq) {
	Bitboard piece = square_bit(sq);

	Bitboard rank = RankBits << (sq & 0b111000);
	Bitboard file = FileBits << (sq & 0b000111);

	Bitboard south = file & (piece - 1);
	Bitboard west = rank & (piece - 1);
	Bitboard north = file ^ south ^ piece;
	Bitboard east = rank ^ west ^ piece;

	Bitboard board = 0;
	int idx = 0;
	do {
		Bitboard moves = 0;

		if (west & board)
			moves |= west & ~(square_bit(__bsrq(west & board)) - 1);
		else
			moves |= west;
		if (south & board)
			moves |= south & ~(square_bit(__bsrq(south & board)) - 1);
		else
			moves |= south;

		moves |= east & _blsmsk_u64(east & board);
		moves |= north & _blsmsk_u64(north & board);

		rook_movetable[sq][idx++] = moves;
		board = (board - rook_masks[sq]) & rook_masks[sq];
	} while (board);
}

void gen_bishop_moves(int sq) {
	Bitboard piece = square_bit(sq);

	int shift = (sq & 0b111) - (sq >> 3);
	Bitboard diag;
	if (shift >= 0)
		diag = (DiagBits >> (shift * 8));
	else
		diag = (DiagBits << (-shift * 8));
	Bitboard anti;
	shift = 7 - (sq & 0b111) - (sq >> 3);
	if (shift >= 0)
		anti = (AntiBits >> (shift * 8));
	else
		anti = (AntiBits << (-shift * 8));
	Bitboard sw = diag & (piece - 1);
	Bitboard se = anti & (piece - 1);
	Bitboard ne = diag ^ sw ^ piece;
	Bitboard nw = anti ^ se ^ piece;

	Bitboard board = 0;
	int idx = 0;
	do {
		Bitboard moves = 0;

		if (sw & board)
			moves |= sw & ~(square_bit(__bsrq(sw & board)) - 1);
		else
			moves |= sw;
		if (se & board)
			moves |= se & ~(square_bit(__bsrq(se & board)) - 1);
		else
			moves |= se;

		moves |= ne & _blsmsk_u64(ne & board);
		moves |= nw & _blsmsk_u64(nw & board);

		bishop_movetable[sq][idx++] = moves;
		board = (board - bishop_masks[sq]) & bishop_masks[sq];
	} while (board);
}

__attribute__((constructor)) void init_movetables() {
	int sq = 0;
	for (Bitboard p = 1; p; p <<= 1) {
		// Rook
		Bitboard rank = RankBits << (sq & 0b111000);
		Bitboard file = FileBits << (sq & 0b000111);

		rook_masks[sq] = rank ^ file;
		gen_rook_moves(sq);

		// Bishop
		int shift = (sq & 0b111) - (sq >> 3);
		Bitboard diag;
		if (shift >= 0)
			diag = (DiagBits >> (shift * 8));
		else
			diag = (DiagBits << (-shift * 8));
		Bitboard anti;
		shift = 7 - (sq & 0b111) - (sq >> 3);
		if (shift >= 0)
			anti = (AntiBits >> (shift * 8));
		else
			anti = (AntiBits << (-shift * 8));

		bishop_masks[sq] = diag ^ anti;
		gen_bishop_moves(sq);

		// Knight
		Bitboard hor1 = ((p << 1) & 0xfefefefefefefefe) | ((p >> 1) & 0x7f7f7f7f7f7f7f7f);
		Bitboard hor2 = ((p << 2) & 0xfcfcfcfcfcfcfcfc) | ((p >> 2) & 0x3f3f3f3f3f3f3f3f);
		knight_movetable[sq] = (hor1 << 16) | (hor1 >> 16) | (hor2 << 8) | (hor2 >> 8);

		// King
		Bitboard king = (hor1 | p);
		king_movetable[sq] = (king | (king << 8) | (king >> 8)) ^ p;

		sq++;
	}
}

void add_moves(int sq, Bitboard dsts, rip::vector<Move> &v) {
	while (dsts) {
		int dst = _tzcnt_u64(dsts);
		v.push_back(Move(sq, dst));
		dsts = _blsr_u64(dsts);
	}
}

void rook_moves(const Position &p, rip::vector<Move> &v) {
	uint64_t occ = p.piece_boards[OCC(WHITE)] | p.piece_boards[OCC(BLACK)];

	Bitboard board = (p.piece_boards[ROOK] | p.piece_boards[QUEEN]) & p.piece_boards[OCC(p.side)];
	while (board) {
		int sq = _tzcnt_u64(board);
		Bitboard dsts = rook_movetable[sq][_pext_u64(occ, rook_masks[sq])] & ~p.piece_boards[OCC(p.side)];
		add_moves(sq, dsts, v);
		board = _blsr_u64(board);
	}
}

void bishop_moves(const Position &p, rip::vector<Move> &v) {
	uint64_t occ = p.piece_boards[OCC(WHITE)] | p.piece_boards[OCC(BLACK)];

	Bitboard board = (p.piece_boards[BISHOP] | p.piece_boards[QUEEN]) & p.piece_boards[OCC(p.side)];
	while (board) {
		int sq = _tzcnt_u64(board);
		Bitboard dsts = bishop_movetable[sq][_pext_u64(occ, bishop_masks[sq])] & ~p.piece_boards[OCC(p.side)];
		add_moves(sq, dsts, v);
		board = _blsr_u64(board);
	}
}

void knight_moves(const Position &p, rip::vector<Move> &v) {
	Bitboard board = p.piece_boards[KNIGHT] & p.piece_boards[OCC(p.side)];
	while (board) {
		int sq = _tzcnt_u64(board);
		Bitboard dsts = knight_movetable[sq] & ~p.piece_boards[OCC(p.side)];
		add_moves(sq, dsts, v);
		board = _blsr_u64(board);
	}
}

void king_moves(const Position &p, rip::vector<Move> &v) {
	Bitboard board = p.piece_boards[KING] & p.piece_boards[OCC(p.side)];
	while (board) {
		int sq = _tzcnt_u64(board);
		Bitboard dsts = king_movetable[sq] & ~p.piece_boards[OCC(p.side)];
		add_moves(sq, dsts, v);
		board = _blsr_u64(board);
	}

	// Castling
	Bitboard occ = p.piece_boards[OCC(WHITE)] | p.piece_boards[OCC(BLACK)];
	if (p.side == WHITE && p.castling & (WHITE_OO | WHITE_OOO) && !p.control(4, BLACK)) {
		if (p.castling & WHITE_OO) {
			if ((occ & (square_bit(5) | square_bit(6))) == 0 && !p.control(5, BLACK) && !p.control(6, BLACK)) {
				v.push_back(Move::make<CASTLING>(4, 6));
			}
		}
		if (p.castling & WHITE_OOO) {
			if ((occ & (square_bit(1) | square_bit(2) | square_bit(3))) == 0 && !p.control(3, BLACK) && !p.control(2, BLACK)) {
				v.push_back(Move::make<CASTLING>(4, 2));
			}
		}
	} else if (p.side == BLACK && p.castling & (BLACK_OO | BLACK_OOO) && !p.control(60, WHITE)) {
		if (p.castling & BLACK_OO) {
			if ((occ & (square_bit(61) | square_bit(62))) == 0 && !p.control(61, WHITE) && !p.control(62, WHITE)) {
				v.push_back(Move::make<CASTLING>(60, 62));
			}
		}
		if (p.castling & BLACK_OOO) {
			if ((occ & (square_bit(57) | square_bit(58) | square_bit(59))) == 0 && !p.control(58, WHITE) && !p.control(59, WHITE)) {
				v.push_back(Move::make<CASTLING>(60, 58));
			}
		}
	}
}

void white_pawn_moves(const Position &p, rip::vector<Move> &v) {
	Bitboard board = p.piece_boards[PAWN] & p.piece_boards[OCC(WHITE)];
	Bitboard dsts;

	// En passant
	if (p.ep_square != SQ_NONE) {
		if ((p.ep_square & 0b111) != 7 && (board & square_bit(p.ep_square - 7)))
			v.push_back(Move::make<EN_PASSANT>(p.ep_square - 7, p.ep_square));
		if ((p.ep_square & 0b111) != 0 && (board & square_bit(p.ep_square - 9)))
			v.push_back(Move::make<EN_PASSANT>(p.ep_square - 9, p.ep_square));
	}

	// East captures
	dsts = ((board & ~(FileBits << 7)) << 9) & p.piece_boards[OCC(BLACK)];
	while (dsts) {
		int dst = _tzcnt_u64(dsts);
		if (dst >= 0b111000) { // promotion
			v.push_back(Move::make<PROMOTION>(dst - 9, dst, QUEEN));
			v.push_back(Move::make<PROMOTION>(dst - 9, dst, ROOK));
			v.push_back(Move::make<PROMOTION>(dst - 9, dst, KNIGHT));
			v.push_back(Move::make<PROMOTION>(dst - 9, dst, BISHOP));
		} else {
			v.push_back(Move(dst - 9, dst));
		}
		dsts = _blsr_u64(dsts);
	}
	// West captures
	dsts = ((board & ~FileBits) << 7) & p.piece_boards[OCC(BLACK)];
	while (dsts) {
		int dst = _tzcnt_u64(dsts);
		if (dst >= 0b111000) { // promotion
			v.push_back(Move::make<PROMOTION>(dst - 7, dst, QUEEN));
			v.push_back(Move::make<PROMOTION>(dst - 7, dst, ROOK));
			v.push_back(Move::make<PROMOTION>(dst - 7, dst, KNIGHT));
			v.push_back(Move::make<PROMOTION>(dst - 7, dst, BISHOP));
		} else {
			v.push_back(Move(dst - 7, dst));
		}
		dsts = _blsr_u64(dsts);
	}

	// Single push
	dsts = (board << 8) & ~(p.piece_boards[OCC(WHITE)] | p.piece_boards[OCC(BLACK)]);
	Bitboard saved_dsts = dsts;
	while (dsts) {
		int dst = _tzcnt_u64(dsts);
		if (dst >= 0b111000) { // promotion
			v.push_back(Move::make<PROMOTION>(dst - 8, dst, QUEEN));
			v.push_back(Move::make<PROMOTION>(dst - 8, dst, ROOK));
			v.push_back(Move::make<PROMOTION>(dst - 8, dst, KNIGHT));
			v.push_back(Move::make<PROMOTION>(dst - 8, dst, BISHOP));
		} else {
			v.push_back(Move(dst - 8, dst));
		}
		dsts = _blsr_u64(dsts);
	}

	// Double push
	dsts = (saved_dsts << 8) & ~(p.piece_boards[OCC(WHITE)] | p.piece_boards[OCC(BLACK)]) & (RankBits << (3 * 8));
	while (dsts) {
		int dst = _tzcnt_u64(dsts);
		v.push_back(Move(dst - 16, dst));
		dsts = _blsr_u64(dsts);
	}
}

void black_pawn_moves(const Position &p, rip::vector<Move> &v) {
	Bitboard board = p.piece_boards[PAWN] & p.piece_boards[OCC(BLACK)];
	Bitboard dsts;

	// En passant
	if (p.ep_square != SQ_NONE) {
		if ((p.ep_square & 0b111) != 7 && (board & square_bit(p.ep_square + 9)))
			v.push_back(Move::make<EN_PASSANT>(p.ep_square + 9, p.ep_square));
		if ((p.ep_square & 0b111) != 0 && (board & square_bit(p.ep_square + 7)))
			v.push_back(Move::make<EN_PASSANT>(p.ep_square + 7, p.ep_square));
	}

	// East captures
	dsts = ((board & ~(FileBits << 7)) >> 7) & p.piece_boards[OCC(WHITE)];
	while (dsts) {
		int dst = _tzcnt_u64(dsts);
		if (dst <= 0b000111) { // promotion
			v.push_back(Move::make<PROMOTION>(dst + 7, dst, QUEEN));
			v.push_back(Move::make<PROMOTION>(dst + 7, dst, ROOK));
			v.push_back(Move::make<PROMOTION>(dst + 7, dst, KNIGHT));
			v.push_back(Move::make<PROMOTION>(dst + 7, dst, BISHOP));
		} else {
			v.push_back(Move(dst + 7, dst));
		}
		dsts = _blsr_u64(dsts);
	}
	// West captures
	dsts = ((board & ~FileBits) >> 9) & p.piece_boards[OCC(WHITE)];
	while (dsts) {
		int dst = _tzcnt_u64(dsts);
		if (dst <= 0b000111) { // promotion
			v.push_back(Move::make<PROMOTION>(dst + 9, dst, QUEEN));
			v.push_back(Move::make<PROMOTION>(dst + 9, dst, ROOK));
			v.push_back(Move::make<PROMOTION>(dst + 9, dst, KNIGHT));
			v.push_back(Move::make<PROMOTION>(dst + 9, dst, BISHOP));
		} else {
			v.push_back(Move(dst + 9, dst));
		}
		dsts = _blsr_u64(dsts);
	}

	// Single push
	dsts = (board >> 8) & ~(p.piece_boards[OCC(WHITE)] | p.piece_boards[OCC(BLACK)]);
	Bitboard saved_dsts = dsts;
	while (dsts) {
		int dst = _tzcnt_u64(dsts);
		if (dst <= 0b000111) { // promotion
			v.push_back(Move::make<PROMOTION>(dst + 8, dst, QUEEN));
			v.push_back(Move::make<PROMOTION>(dst + 8, dst, ROOK));
			v.push_back(Move::make<PROMOTION>(dst + 8, dst, KNIGHT));
			v.push_back(Move::make<PROMOTION>(dst + 8, dst, BISHOP));
		} else {
			v.push_back(Move(dst + 8, dst));
		}
		dsts = _blsr_u64(dsts);
	}

	// Double push
	dsts = (saved_dsts >> 8) & ~(p.piece_boards[OCC(WHITE)] | p.piece_boards[OCC(BLACK)]) & (RankBits << (4 * 8));
	while (dsts) {
		int dst = _tzcnt_u64(dsts);
		v.push_back(Move(dst + 16, dst));
		dsts = _blsr_u64(dsts);
	}
}

void pawn_moves(const Position &p, rip::vector<Move> &v) {
	if (p.side == WHITE) {
		white_pawn_moves(p, v);
	} else {
		black_pawn_moves(p, v);
	}
}

void Position::legal_moves(rip::vector<Move> &v) const {
	rook_moves(*this, v);
	bishop_moves(*this, v);
	knight_moves(*this, v);
	pawn_moves(*this, v);
	king_moves(*this, v);
}

bool Position::control(int sq, bool side) const {
	uint64_t occ = piece_boards[OCC(WHITE)] | piece_boards[OCC(BLACK)];

	Bitboard dsts = rook_movetable[sq][_pext_u64(occ, rook_masks[sq])];
	if (dsts & (piece_boards[ROOK] | piece_boards[QUEEN]) & piece_boards[OCC(side)])
		return true;

	dsts = bishop_movetable[sq][_pext_u64(occ, bishop_masks[sq])];
	if (dsts & (piece_boards[BISHOP] | piece_boards[QUEEN]) & piece_boards[OCC(side)])
		return true;

	if (knight_movetable[sq] & piece_boards[KNIGHT] & piece_boards[OCC(side)])
		return true;

	if (((square_bit(sq - 9) & ~(FileBits << 7)) | (square_bit(sq - 7) & ~FileBits)) & piece_boards[PAWN] & piece_boards[OCC(WHITE)] & piece_boards[OCC(side)])
		return true;
	if (((square_bit(sq + 7) & ~(FileBits << 7)) | (square_bit(sq + 9) & ~FileBits)) & piece_boards[PAWN] & piece_boards[OCC(BLACK)] & piece_boards[OCC(side)])
		return true;

	if (king_movetable[sq] & piece_boards[KING] & piece_boards[OCC(side)])
		return true;

	return false;
}
