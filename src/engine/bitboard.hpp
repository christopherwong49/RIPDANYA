#pragma once

#include "defines.hpp"

typedef uint64_t Bitboard;

constexpr Bitboard FileBits = 0x0101010101010101;
constexpr Bitboard RankBits = 0xff;
constexpr Bitboard DiagBits = 0x8040201008040201;
constexpr Bitboard AntiBits = 0x0102040810204080;

constexpr Bitboard square_bit(int x) {
	return 1ULL << x;
}
