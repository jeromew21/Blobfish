#ifndef PIECES_HPP
#define PIECES_HPP

#include <game/glob.hpp>

const PieceType W_Pawn = 0;
const PieceType W_Knight = 1;
const PieceType W_Bishop = 2;
const PieceType W_Rook = 3;
const PieceType W_Queen = 4;
const PieceType W_King = 5;

const PieceType B_Pawn = 6;
const PieceType B_Knight = 7;
const PieceType B_Bishop = 8;
const PieceType B_Rook = 9;
const PieceType B_Queen = 10;
const PieceType B_King = 11;

const PieceType Empty = 12;

const int MATERIAL_TABLE[13] = {100, 350, 351, 525, 1000, 10000,
                                100, 350, 351, 525, 1000, 10000, 0};

std::string pieceToString(PieceType piece);
std::string pieceToStringAlph(PieceType piece);
std::string pieceToStringFen(PieceType piece);
std::string pieceToStringAlphLower(PieceType piece);
PieceType pieceFromString(std::string val);

inline Color flipColor(Color color) { return 1 & (~color); }

Color colorOf(PieceType piece);
PieceType pieceIndexFromColor(Color color);

#endif
