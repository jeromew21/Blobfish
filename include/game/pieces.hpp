#ifndef PIECES_HPP
#define PIECES_HPP

#include <game/glob.hpp>

const PieceType W_King = 0;
const PieceType W_Pawn = 1;
const PieceType W_Knight = 2;
const PieceType W_Bishop = 3;
const PieceType W_Rook = 4;
const PieceType W_Queen = 5;

const PieceType B_King = 6;
const PieceType B_Pawn = 7;
const PieceType B_Knight = 8;
const PieceType B_Bishop = 9;
const PieceType B_Rook = 10;
const PieceType B_Queen = 11;

const PieceType Empty = 12;

const int MATERIAL_TABLE[12] = {10000, 100, 350, 350, 525, 1000, 10000, 100, 350, 350, 525, 1000};

std::string pieceToString(PieceType piece);
std::string pieceToStringAlph(PieceType piece);
std::string pieceToStringFen(PieceType piece);
std::string pieceToStringAlphLower(PieceType piece);
PieceType pieceFromString(std::string val);
Color flipColor(Color color);
Color colorOf(PieceType piece);
PieceType pieceIndexFromColor(Color color);

#endif
