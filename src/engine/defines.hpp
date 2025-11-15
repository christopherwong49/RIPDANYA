#pragma once

#include "stl/vector.hpp"

#include <cstdint>
#include <immintrin.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

#define MAX_PLY (256)

#ifndef __x86_64__
constexpr uint64_t __bsrq(uint64_t x) {
	uint64_t pos = 0;
	while (x >>= 1)
		pos++;
	return pos;
}

constexpr uint64_t _blsmsk_u64(uint64_t x) {
	return x & -x;
}

constexpr uint64_t _blsr_u64(uint64_t x) {
	return x & (x - 1);
}

constexpr uint64_t _tzcnt_u64(uint64_t x) {
	if (x == 0)
		return 64;
	uint64_t pos = 0;
	while ((x & 1) == 0) {
		x >>= 1;
		pos++;
	}
	return pos;
}

constexpr uint64_t _pext_u64(uint64_t src, uint64_t mask) {
	uint64_t result = 0;
	uint64_t bb = 1;
	for (uint64_t bb_mask = 1; mask; bb_mask <<= 1) {
		if (mask & bb_mask) {
			if (src & bb_mask)
				result |= bb;
			bb <<= 1;
			mask &= ~bb_mask;
		}
	}
	return result;
}

constexpr uint64_t _mm_popcnt_u64(uint64_t x) {
	uint64_t count = 0;
	while (x) {
		x &= x - 1;
		count++;
	}
	return count;
}
#endif

enum Color : bool {
	WHITE,
	BLACK,
};

enum Castling : uint8_t {
	NO_CASTLING, // 0
	WHITE_OOO, // 1
	WHITE_OO = WHITE_OOO << 1, // 2
	BLACK_OOO = WHITE_OO << 1, // 4
	BLACK_OO = BLACK_OOO << 1, // 8
};

typedef int16_t Value;
constexpr Value VALUE_INFINITE = 32000;
constexpr Value VALUE_MATE = 30000;
constexpr Value VALUE_MATE_MAX_PLY = VALUE_MATE - MAX_PLY;

constexpr Value PawnValue = 100;
constexpr Value KnightValue = 300;
constexpr Value BishopValue = 350;
constexpr Value RookValue = 500;
constexpr Value QueenValue = 900;
constexpr Value ScaleValue = (QueenValue + PawnValue - 1) / PawnValue;

constexpr Value PieceValues[] = {PawnValue, KnightValue, BishopValue, RookValue, QueenValue, VALUE_INFINITE};

enum PieceType : uint8_t {
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING,
	NO_PIECETYPE,
};

enum Piece : uint8_t {
	WHITE_PAWN,
	WHITE_KNIGHT,
	WHITE_BISHOP,
	WHITE_ROOK,
	WHITE_QUEEN,
	WHITE_KING,
	BLACK_PAWN = 8,
	BLACK_KNIGHT,
	BLACK_BISHOP,
	BLACK_ROOK,
	BLACK_QUEEN,
	BLACK_KING,
	NO_PIECE,
};

constexpr PieceType letter_piece[] = {BISHOP, NO_PIECETYPE, NO_PIECETYPE, NO_PIECETYPE, NO_PIECETYPE, NO_PIECETYPE, NO_PIECETYPE, NO_PIECETYPE, NO_PIECETYPE,
									  KING,	  NO_PIECETYPE, NO_PIECETYPE, KNIGHT,		NO_PIECETYPE, PAWN,			QUEEN,		  ROOK};

constexpr char piecetype_letter[] = "pnbrqk?";
constexpr char piece_letter[] = "PNBRQK??pnbrqk.";
