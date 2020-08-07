#ifndef GLOB_HPP
#define GLOB_HPP

#include <array>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#define PieceType int
#define Color int
#define NodeType int16_t

#define u64 uint64_t

const int ROOK_UP = 0;
const int ROOK_RIGHT = 1;
const int ROOK_DOWN = 2;
const int ROOK_LEFT = 3;

const NodeType PV = 0;
const NodeType Cut = 1;
const NodeType All = 2;

const int SCORE_MIN = -10000000; // minimum value
const int SCORE_MAX = 10000000;

const int BOARD_STATE_ENTROPY = 9;

const int TURN_INDEX = 0;
const int EN_PASSANT_INDEX = 1;
const int W_LONG_INDEX = 2;
const int W_SHORT_INDEX = 3;
const int B_LONG_INDEX = 4;
const int B_SHORT_INDEX = 5;
const int LAST_MOVED_INDEX = 6;
const int LAST_CAPTURED_INDEX = 7;
const int HAS_REPEATED_INDEX = 8;

const Color White = 0;
const Color Black = 1;
const Color Neutral = -1;

enum class BoardStatus {
  WhiteWin = 1,
  BlackWin = -1,
  Draw = 0,
  Playing = 2,
  Stalemate = 3,
  NotCalculated = 4,
};

const std::string RANK_NAMES[] = {"1", "2", "3", "4", "5", "6", "7", "8"};
const std::string FILE_NAMES[] = {"a", "b", "c", "d", "e", "f", "g", "h"};

void sendCommand(const std::string &cmd);

void debugLog(const std::string &f);
std::string yesorno(bool b);
std::string statusToString(BoardStatus bs, bool concise);
std::string colorToString(Color c);

std::string squareName(u64 square);
std::string squareName(int square);

int indexFromSquareName(std::string alg);

u64 u64FromPair(int r, int c);
int intFromPair(int r, int c);

int distToClosestCorner(int r, int c);

// LSB (rightmost, uppermost)
inline int bitscanForward(u64 x) { // checked, should work
  return __builtin_ffsll(x) - 1;
}

// MSB (leftmost, uppermost)
inline int bitscanReverse(u64 x) { return 63 - __builtin_clzll(x); }

inline u64 u64FromIndex(int i) { // fixed, should work
  return (1UL) << i;
}

inline int u64ToIndex(u64 space) { return bitscanForward(space); }

void dump64(u64 x);

int hadd(u64 x);

inline int max(int i1, int i2) {
  if (i1 > i2) {
    return i1;
  } else {
    return i2;
  }
}

inline int min(int i1, int i2) {
  if (i1 < i2) {
    return i1;
  } else {
    return i2;
  }
}

void bitscanAll(std::array<u64, 64> &arr, u64 x, int &outsize);    // hotspot
void bitscanAllInt(std::array<int, 64> &arr, u64 x, int &outsize); // hotspot

int rand100();
void srand100(int seed);

std::vector<std::string> tokenize(std::string instring);

inline bool inBounds(int y, int x) {
  return (y >= 0 && y < 8) && (x >= 0 && x < 8);
}

inline int u64ToRow(u64 space) { return bitscanForward(space) / 8; }

inline int u64ToCol(u64 space) { return bitscanForward(space) % 8; }

inline int intToRow(int s) { return s / 8; }
inline int intToCol(int s) { return s % 8; }

inline u64 u64FromPair(int r, int c) { return u64FromIndex(r * 8 + c); }

inline int intFromPair(int r, int c) { return r * 8 + c; }

#endif
