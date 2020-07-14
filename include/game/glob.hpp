#ifndef GLOB_HPP
#define GLOB_HPP

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <random>
#include <sstream>
#include <iterator>

#define PieceType u_int8_t
#define Color int

#define u64 u_int64_t

const int INTMIN = -10000000; // minimum value
const int INTMAX =  10000000;

const size_t BOARD_STATE_ENTROPY = 6;

const size_t TURN_INDEX = 0;
const size_t EN_PASSANT_INDEX = 1;
const size_t W_LONG_INDEX = 2;
const size_t W_SHORT_INDEX = 3;
const size_t B_LONG_INDEX = 4;
const size_t B_SHORT_INDEX = 5;

const Color White = 0;
const Color Black = 1;
const Color Neutral = -1;


enum class BoardStatus {
    WhiteWin = 1, BlackWin = -1,
    Draw = 0, Playing = 2, Stalemate = 3
};

const std::string RANK_NAMES[] = {"1", "2", "3", "4", "5", "6", "7", "8"};
const std::string FILE_NAMES[] = {"a", "b", "c", "d", "e", "f", "g", "h"};

void sendCommand(const std::string& cmd);

void debugLog(const std::string& f);
std::string yesorno(bool b);
std::string statusToString(BoardStatus bs, bool concise);
std::string colorToString(Color c);

std::string squareName(u64 square);
std::string squareName(int square);

int indexFromSquareName(std::string alg);

u64 u64FromPair(int r, int c);
u64 u64FromIndex(int i);

int u64ToRow(u64 space);
int u64ToCol(u64 space);
int u64ToIndex(u64 space);

void dump64(u64 x);

bool inBounds(int y, int x);

int bitscanForward(u64 x);
int bitscanReverse(u64 x);
int hadd(u64 x);

int max(int i1, int i2);
int min(int i1, int i2);

std::vector<u64> bitscanAll(u64 x); //hotspot
std::array<u64, 64> bitscanAll(u64 x, int &outsize); //hotspot

int rand100();
void srand100(int seed);

std::vector<std::string> tokenize(std::string instring);

#endif
