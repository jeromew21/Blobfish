#include <string>

#include <game/pieces.hpp>

Color colorOf(PieceType piece) {
  if (piece == Empty) {
    return Neutral;
  }
  if (piece < 6) {
    return White;
  } else {
    return Black;
  }
}

PieceType pieceFromString(std::string val) {
  if (val == "K") {
    return W_King;
  } else if (val == "Q") {
    return W_Queen;
  } else if (val == "B") {
    return W_Bishop;
  } else if (val == "R") {
    return W_Rook;
  } else if (val == "N") {
    return W_Knight;
  } else if (val == "P") {
    return W_Pawn;
  } else if (val == "k") {
    return B_King;
  } else if (val == "q") {
    return B_Queen;
  } else if (val == "b") {
    return B_Bishop;
  } else if (val == "r") {
    return B_Rook;
  } else if (val == "n") {
    return B_Knight;
  } else if (val == "p") {
    return B_Pawn;
  }
  return Empty;
}

std::string pieceToStringFen(PieceType piece) {
  switch (piece) {
  case W_King:
    return "K";
  case B_King:
    return "k";
  case W_Queen:
    return "Q";
  case B_Queen:
    return "q";
  case W_Rook:
    return "R";
  case B_Rook:
    return "r";
  case W_Knight:
    return "N";
  case B_Knight:
    return "n";
  case W_Bishop:
    return "B";
  case B_Bishop:
    return "b";
  case W_Pawn:
    return "P";
  case B_Pawn:
    return "p";
  default:
    return "?";
  }
}

std::string pieceToString(PieceType piece) {
  switch (piece) {
  case W_King:
    return "♔";
  case W_Queen:
    return "♕";
  case W_Rook:
    return "♖";
  case W_Knight:
    return "♘";
  case W_Bishop:
    return "♗";
  case W_Pawn:
    return "♙";

  case B_King:
    return "♚";
  case B_Queen:
    return "♛";
  case B_Rook:
    return "♜";
  case B_Knight:
    return "♞";
  case B_Bishop:
    return "♝";
  case B_Pawn:
    return "♟";
  default:
    return " ";
  }
}


std::string pieceToStringAlphLower(PieceType piece) {
  switch (piece) {
  case W_King:
  case B_King:
    return "k";
  case W_Queen:
  case B_Queen:
    return "q";
  case W_Rook:
  case B_Rook:
    return "r";
  case W_Knight:
  case B_Knight:
    return "n";
  case W_Bishop:
  case B_Bishop:
    return "b";
  case W_Pawn:
  case B_Pawn:
    return "p";
  default:
    return "";
  }
}

std::string pieceToStringAlph(PieceType piece) {
  switch (piece) {
  case W_King:
  case B_King:
    return "K";
  case W_Queen:
  case B_Queen:
    return "Q";
  case W_Rook:
  case B_Rook:
    return "R";
  case W_Knight:
  case B_Knight:
    return "N";
  case W_Bishop:
  case B_Bishop:
    return "B";
  case W_Pawn:
  case B_Pawn:
    return "P";
  default:
    std::cout << "unknown piece " << (int)(piece) << "\n";
    return "?";
  }
}
