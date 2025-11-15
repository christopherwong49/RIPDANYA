#include "incbin.h"

#include "nnue.hpp"

#include <algorithm>
#include <cstring>

#ifndef NNUE_PATH
#define NNUE_PATH "nnue.bin"
#endif

extern "C" {
INCBIN(network_weights, NNUE_PATH);
}

#define HL_SIZE 1024
#define QA 255
#define QB 64
#define SCALE 400
#define NINPUTS 16
#define NOUTPUTS 8

// int16_t *accumulator_weights = (int16_t *)gnetwork_weightsData;
// int16_t *accumulator_biases = accumulator_weights + (768 * NBUCKETS) * (HL_SIZE);
// int16_t *output_weights = accumulator_biases + (HL_SIZE);
// int16_t *output_bias = output_weights + (8) * (HL_SIZE * 2);

namespace net {
	static int16_t accumulator_weights[768 * NINPUTS][HL_SIZE];
	static int16_t accumulator_biases[HL_SIZE];
	static int16_t output_weights[NOUTPUTS][2 * HL_SIZE];
	static int16_t output_bias[NOUTPUTS];
};

__attribute__((constructor)) void load_nnue() {
	size_t offset = 0;

	size_t size = sizeof(net::accumulator_weights);
	memcpy(net::accumulator_weights, gnetwork_weightsData + offset, size);
	offset += size;

	size = sizeof(net::accumulator_biases);
	std::memcpy(net::accumulator_biases, gnetwork_weightsData + offset, size);
	offset += size;

	size = sizeof(net::output_weights);
	std::memcpy(net::output_weights, gnetwork_weightsData + offset, size);
	offset += size;

	size = sizeof(net::output_bias);
	std::memcpy(net::output_bias, gnetwork_weightsData + offset, size);
	offset += size;
}

int calculate_index(int sq, PieceType pt, bool side, bool perspective, int nbucket) {
	if (nbucket & 1) {
		// Mirroring
		sq = sq ^ 7;
	}
	nbucket /= 2;
	if (perspective) {
		// Flip board
		side = !side;
		sq = sq ^ 56;
	}
	return nbucket * 768 + side * 64 * 6 + pt * 64 + sq;
}

void accumulator_add(Accumulator &acc, int index) {
	for (int i = 0; i < HL_SIZE; i++) {
		acc.val[i] += net::accumulator_weights[index][i];
	}
}

void accumulator_sub(Accumulator &acc, int index) {
	for (int i = 0; i < HL_SIZE; i++) {
		acc.val[i] -= net::accumulator_weights[index][i];
	}
}

int32_t nnue_eval(const Accumulator &stm, const Accumulator &ntm, uint8_t nbucket) {
	int32_t score = 0;

	for (int i = 0; i < HL_SIZE; i++) {
		int32_t v1 = std::clamp(stm.val[i], (int16_t)0, (int16_t)QA);
		int32_t v2 = std::clamp(ntm.val[i], (int16_t)0, (int16_t)QA);

		score += v1 * v1 * net::output_weights[nbucket][i];
		score += v2 * v2 * net::output_weights[nbucket][HL_SIZE + i];
	}

	score /= QA;
	score += net::output_bias[nbucket];
	score *= SCALE;
	score /= QA * QB;
	return score;
}

// Accumulator stm_accs[16 * 16];
// Accumulator ntm_accs[16 * 16];

// clang-format off
uint8_t bucket_layout[64] = {
	0, 2, 4, 6, 7, 5, 3, 1,
	8, 10, 12, 14, 15, 13, 11, 9,
	16, 16, 18, 18, 19, 19, 17, 17,
	20, 20, 22, 22, 23, 23, 21, 21,
	24, 24, 26, 26, 27, 27, 25, 25,
	24, 24, 26, 26, 27, 27, 25, 25,
	28, 28, 30, 30, 31, 31, 29, 29,
	28, 28, 30, 30, 31, 31, 29, 29,
};
// clang-format on

Value eval(Position &p) {
	int nbucket = (_mm_popcnt_u64(p.piece_boards[OCC(WHITE)] | p.piece_boards[OCC(BLACK)]) - 2) / 4;

	int wbucket = bucket_layout[_tzcnt_u64(p.piece_boards[OCC(WHITE)] & p.piece_boards[KING])];
	int bbucket = bucket_layout[0b111000 ^ _tzcnt_u64(p.piece_boards[OCC(BLACK)] & p.piece_boards[KING])];

	Accumulator w_acc, b_acc;

	for (int i = 0; i < HL_SIZE; i++) {
		w_acc.val[i] = net::accumulator_biases[i];
		b_acc.val[i] = net::accumulator_biases[i];
	}

	for (int sq = 0; sq < 64; sq++) {
		Piece piece = p.mailbox[sq];
		if (piece == NO_PIECE)
			continue;

		PieceType pt = PieceType(piece & 7);
		bool side = piece >> 3;

		int windex = calculate_index(sq, pt, side, WHITE, wbucket);
		int bindex = calculate_index(sq, pt, side, BLACK, bbucket);

		accumulator_add(w_acc, windex);
		accumulator_add(b_acc, bindex);
	}

	if (p.side == WHITE)
		return nnue_eval(w_acc, b_acc, nbucket);
	else
		return nnue_eval(b_acc, w_acc, nbucket);
}
