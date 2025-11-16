#pragma once

#include "defines.hpp"

enum MoveType {
	NORMAL,
	CASTLING = 1 << 14,
	EN_PASSANT = 2 << 14,
	PROMOTION = 3 << 14,
};

struct Position;

struct Move {
	uint16_t data;
	Move() : data(0) {}
	constexpr Move(uint16_t x) : data(x) {}
	constexpr Move(uint8_t src, uint8_t dst) : data((src << 6) | dst) {}

	template <MoveType T> static constexpr Move make(int src, int dst, PieceType pt = QUEEN) {
		return Move(T | ((pt - KNIGHT) << 12) | (src << 6) | dst);
	}

	constexpr int src() const { // source
		return (data >> 6) & 0b111111;
	}
	constexpr int dst() const { // destination
		return data & 0b111111;
	}
	constexpr PieceType promo() const {
		return (PieceType)(((data >> 12) & 0b11) + KNIGHT);
	}
	constexpr MoveType type() const {
		return (MoveType)(data & 0xc000);
	}
	constexpr bool operator==(Move &o) const {
		return o.data == data;
	}
	constexpr bool operator!=(Move &o) const {
		return o.data != data;
	}

	constexpr bool operator<(const Move &o) const {
		return data > o.data;
	}

	std::string to_string() const;
	static Move from_string(const std::string &s, Position &p);
};

constexpr Move NullMove = Move(0);
