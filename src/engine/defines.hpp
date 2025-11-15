#pragma once

#include "stl/vector.hpp"

#include <cstdint>
#include <immintrin.h>

#include <iostream>
#include <sstream>
#include <string>

#define MAX_PLY (256)

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

typedef uint16_t Value;
constexpr Value VALUE_INFINITE = 32000;
constexpr Value VALUE_MATE = 30000;
constexpr Value VALUE_MATE_MAX_PLY = VALUE_MATE - MAX_PLY;

constexpr Value PawnValue = 100;
constexpr Value KnightValue = 300;
constexpr Value BishopValue = 350;
constexpr Value RookValue = 500;
constexpr Value QueenValue = 900;
constexpr Value ScaleValue = (QueenValue + PawnValue - 1) / PawnValue;

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

constexpr char piecetype_letter[] = "pnbqrk?";
constexpr char piece_letter[] = "PNBRQK??pnbrqk.";
