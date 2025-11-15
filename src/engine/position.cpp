#include "position.hpp"
#include <cctype>
#include <iostream>
#include <sstream>

uint64_t zobrist_square[64][15];
uint64_t zobrist_castling[16];
uint64_t zobrist_ep[8];
uint64_t zobrist_side;

__attribute__((constructor)) void init_zobrist() {
	std::srand(0xdeadbeef);
	for (int sq = 0; sq < 64; sq++) {
		for (int piece = 0; piece < 15; piece++) {
			zobrist_square[sq][piece] = ((uint64_t)std::rand() << 32) | std::rand();
		}
	}

	for (int i = 0; i < 16; i++)
		zobrist_castling[i] = ((uint64_t)std::rand() << 32) | std::rand();

	for (int i = 0; i < 8; i++)
		zobrist_ep[i] = ((uint64_t)std::rand() << 32) | std::rand();

	zobrist_side = ((uint64_t)std::rand() << 32) | std::rand();
}

void Position::load_fen(std::string fen) {
	// clear board
	for (int i = 0; i < 8; i++) {
		piece_boards[i] = 0ULL;
	}

	// clear mailbox
	for (int i = 0; i < 64; i++) {
		mailbox[i] = NO_PIECE;
	}

	std::istringstream ss(fen);

	std::string board, turn, castle, enpassant;
	int full = 1;

	// take in input
	ss >> board >> turn >> castle >> enpassant >> halfmove >> full;

	// pieces
	int square = 0b111000;
	for (char c : board) {
		if (c == '/') {
			square -= 16;
		} else if (isdigit(c)) { // skip empty squares
			square += c - '0';
		} else {
			Piece piece = NO_PIECE;
			if (c & 0x20)
				piece = Piece(8 + letter_piece[c - 'b']);
			else
				piece = Piece(letter_piece[c - 'B']);

			// set piece
			piece_boards[piece & 7] ^= square_bit(square);
			piece_boards[OCC(piece >> 3)] ^= square_bit(square);
			mailbox[square] = piece;

			square++;
		}
	}

	// side
	side = (turn == "w") ? WHITE : BLACK; // 0 : 1

	// castling
	castling = 0;
	if (castle.find('Q') != std::string::npos)
		castling |= WHITE_OOO;
	if (castle.find('K') != std::string::npos)
		castling |= WHITE_OO;
	if (castle.find('q') != std::string::npos)
		castling |= BLACK_OOO;
	if (castle.find('k') != std::string::npos)
		castling |= BLACK_OO;

	// ep
	if (enpassant != "-") {
		int file = enpassant[0] - 'a';
		int rank = enpassant[1] - '1';
		ep_square = rank * 8 + file;
	} else {
		ep_square = 64;
	}
}

void Position::checkhash(uint64_t testhash) {
	uint64_t cur = testhash;
	recompute_hash();
	if (cur != zobrist) {
		std::cout << "Hash mismatch! 1" << std::endl;
		print_board();
		std::cout << "Given hash: " << cur << std::endl;
		std::cout << "Computed hash: " << zobrist << std::endl;
		exit(1);
	}
}

void Position::make_move(Move move) {
	if (move.data == 0) {
		side = !side;
		return;
	}

	int from = move.src();
	int to = move.dst();

	Piece piece = mailbox[from];

	if (ep_square != SQ_NONE)
		zobrist ^= zobrist_ep[ep_square & 7];
	ep_square = SQ_NONE;

	// capture
	if (mailbox[to] != NO_PIECE || move.type() == EN_PASSANT) {
		int cap = to;
		if (move.type() == EN_PASSANT)
			cap = (from & 0b111000) | (to & 0b000111);

		// remove piece
		piece_boards[mailbox[cap] & 7] ^= square_bit(cap); // piece
		piece_boards[OPPOCC(side)] ^= square_bit(cap); // color: 6 7 6 7

		zobrist ^= zobrist_square[cap][mailbox[cap]];
		mailbox[cap] = NO_PIECE;
	}

	// remove from
	piece_boards[piece & 7] ^= square_bit(from); // piece
	piece_boards[OCC(side)] ^= square_bit(from); // color: 6 7 6 7
	mailbox[from] = NO_PIECE;
	zobrist ^= zobrist_square[from][piece];

	// set piece
	switch (move.type()) {
	case CASTLING:
		if (side == WHITE) {
			zobrist ^= zobrist_square[to][WHITE_KING];
		} else {
			zobrist ^= zobrist_square[to][BLACK_KING];
		}

		if (to == 2) { // to = C1, white queen castle
			// set piece king (assume UCI)
			piece_boards[KING] ^= square_bit(to);
			piece_boards[OCC(WHITE)] ^= square_bit(to);
			mailbox[to] = WHITE_KING;

			// set piece rook
			// remove A1
			piece_boards[ROOK] ^= square_bit(0);
			piece_boards[OCC(WHITE)] ^= square_bit(0);
			mailbox[0] = NO_PIECE;
			// set D1
			piece_boards[ROOK] ^= square_bit(3);
			piece_boards[OCC(WHITE)] ^= square_bit(3);
			mailbox[3] = WHITE_ROOK;
			zobrist ^= zobrist_square[0][WHITE_ROOK] ^ zobrist_square[3][WHITE_ROOK];
		}
		if (to == 6) { // to = G1, white king castle
			// set piece king (assume UCI)
			piece_boards[KING] ^= square_bit(to);
			piece_boards[OCC(WHITE)] ^= square_bit(to);
			mailbox[to] = WHITE_KING;

			// set piece rook
			// remove H1
			piece_boards[ROOK] ^= square_bit(7);
			piece_boards[OCC(WHITE)] ^= square_bit(7);
			mailbox[7] = NO_PIECE;
			// set F1
			piece_boards[ROOK] ^= square_bit(5);
			piece_boards[OCC(WHITE)] ^= square_bit(5);
			mailbox[5] = WHITE_ROOK;
			zobrist ^= zobrist_square[7][WHITE_ROOK] ^ zobrist_square[5][WHITE_ROOK];
		}
		if (to == 58) { // to = C8, black queen castle
			// set piece king (assume UCI)
			piece_boards[KING] ^= square_bit(to);
			piece_boards[OCC(BLACK)] ^= square_bit(to);
			mailbox[to] = BLACK_KING;

			// set piece rook
			// remove A8
			piece_boards[ROOK] ^= square_bit(56);
			piece_boards[OCC(BLACK)] ^= square_bit(56);
			mailbox[56] = NO_PIECE;
			// set D8
			piece_boards[ROOK] ^= square_bit(59);
			piece_boards[OCC(BLACK)] ^= square_bit(59);
			mailbox[59] = BLACK_ROOK;
			zobrist ^= zobrist_square[56][BLACK_ROOK] ^ zobrist_square[59][BLACK_ROOK];
		}
		if (to == 62) { // to = G8, black king castle
			// set piece king (assume UCI)
			piece_boards[KING] ^= square_bit(to);
			piece_boards[OCC(BLACK)] ^= square_bit(to);
			mailbox[to] = BLACK_KING;

			// set piece rook
			// remove H8
			piece_boards[ROOK] ^= square_bit(63);
			piece_boards[OCC(BLACK)] ^= square_bit(63);
			mailbox[63] = NO_PIECE;
			// set F8
			piece_boards[ROOK] ^= square_bit(61);
			piece_boards[OCC(BLACK)] ^= square_bit(61);
			mailbox[61] = BLACK_ROOK;
			zobrist ^= zobrist_square[63][BLACK_ROOK] ^ zobrist_square[61][BLACK_ROOK];
		}
		break;
	case PROMOTION:
		// set piece
		piece_boards[move.promo()] ^= square_bit(to);
		piece_boards[OCC(side)] ^= square_bit(to);
		mailbox[to] = Piece(move.promo() | (side << 3));
		zobrist ^= zobrist_square[to][mailbox[to]];
		break;

	default:
		if (abs(to - from) == 16 && (piece & 7) == PAWN) { // double pawn push
			ep_square = (side == WHITE) ? (to - 8) : (to + 8);
		}
		// set piece
		piece_boards[piece & 7] ^= square_bit(to);
		piece_boards[OCC(side)] ^= square_bit(to);
		mailbox[to] = piece;
		zobrist ^= zobrist_square[to][piece];
		break;
	}

	// castling
	uint8_t old_castling = castling;
	if (piece == WHITE_KING)
		castling &= ~3; // remove white K/Q rights
	if (piece == BLACK_KING)
		castling &= ~12; // remove black k/q rights

	if (from == 0 || to == 0)
		castling &= ~1; // A1: remove white Q rights
	if (from == 7 || to == 7)
		castling &= ~2; // H1: remove white K rights
	if (from == 56 || to == 56)
		castling &= ~4; // A8: remove black q rights
	if (from == 63 || to == 63)
		castling &= ~8; // H8: remove black k rights

	zobrist ^= zobrist_castling[old_castling] ^ zobrist_castling[castling];
	if (ep_square != SQ_NONE)
		zobrist ^= zobrist_ep[ep_square & 7];

	// switch sides
	side = !side;
	zobrist ^= zobrist_side;

#ifdef HASHCHECK // trauma :(
	uint64_t check_hash = zobrist;
	recompute_hash();
	if (check_hash != zobrist) {
		std::cout << "Hash mismatch!" << std::endl;
		print_board();
		std::cout << "Move: " << move.to_string() << std::endl;
		std::cout << "Old hash: " << check_hash << std::endl;
		std::cout << "New hash: " << zobrist << std::endl;
		exit(1);
	}

#endif
}

void Position::print_board() const {
	for (int rank = 7; rank >= 0; rank--) {
		for (int file = 0; file < 8; file++) {
			int sq = rank * 8 + file;
			Piece piece = mailbox[sq];
			std::cout << piece_letter[piece] << ' ';
		}
		std::cout << std::endl;
	}
}

void Position::recompute_hash() {
	zobrist = 0;
	for (int sq = 0; sq < 64; sq++) {
		Piece piece = mailbox[sq];
		if (piece != NO_PIECE) {
			zobrist ^= zobrist_square[sq][piece];
		}
	}
	if (side == BLACK) {
		zobrist ^= zobrist_side;
	}
	if (ep_square != SQ_NONE) {
		zobrist ^= zobrist_ep[ep_square & 0b111];
	}
	zobrist ^= zobrist_castling[castling];
}

std::string Move::to_string() const {
	if (data == 0) {
		return "0000";
	}
	std::string str = "";
	str += ('a' + (src() & 0b111));
	str += ('1' + (src() >> 3));
	str += ('a' + (dst() & 0b111));
	str += ('1' + (dst() >> 3));

	if ((data & 0xc000) == PROMOTION) {
		str += piecetype_letter[promo()];
	}

	return str;
}

Move Move::from_string(const std::string &s, Position &p) {
	int src = ((s[1] - '1') << 3) + (s[0] - 'a');
	int dst = ((s[3] - '1') << 3) + (s[2] - 'a');

	if (s.length() == 5) { // promotion
		PieceType pt = letter_piece[s[4] - 'b'];
		return Move::make<PROMOTION>(src, dst, pt);
	} else {
		// check for en passant
		if ((p.mailbox[src] & 7) == PAWN && dst == p.ep_square) {
			return Move::make<EN_PASSANT>(src, dst);
		}
		// check for castling
		if ((p.mailbox[src] & 7) == KING && abs(dst - src) == 2) {
			return Move::make<CASTLING>(src, dst);
		}
		return Move(src, dst);
	}
}
