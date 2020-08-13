#ifndef MOVE_HPP
#define MOVE_HPP

#include <game/pieces.hpp>

namespace MoveTypeCode {
const int Null = 0;
const int Default = 1;
const int CastleLong = 2;
const int CastleShort = 3;
const int DoublePawn = 4;
const int EnPassant = 5;
const int KPromotion = 6;
const int BPromotion = 7;
const int RPromotion = 8;
const int QPromotion = 9;
} // namespace MoveTypeCode

struct Move {
  uint16_t data;

  static Move NullMove() { return Move(); }

  inline bool isNull() { return getTypeCode() == MoveTypeCode::Null; }

  inline bool notNull() { return getTypeCode() != MoveTypeCode::Null; }

  inline int getTypeCode() { return data & 15; }

  inline PieceType getPromotingPiece() { return getTypeCode() - 5; }

  inline PieceType getPromotingPiece(Color c) {
    return getPromotingPiece() + 6 * c;
  }

  inline bool isPromotion() {
    int tc = getTypeCode();
    return tc >= MoveTypeCode::KPromotion && tc <= MoveTypeCode::QPromotion;
  }

  inline bool isCastle() {
    int tc = getTypeCode();
    return tc == MoveTypeCode::CastleLong || tc == MoveTypeCode::CastleShort;
  }

  inline int getSrcIndex() { return data >> 10; }

  inline int getDestIndex() { return (data >> 4) & 63; }

  inline u64 getSrc() { return u64FromIndex(data >> 10); }

  inline u64 getDest() { return u64FromIndex((data >> 4) & 63); }

  Move(int src0, int dest0, int typeCode) {
    data = (src0 << 10) | (dest0 << 4) | (typeCode & 15);
  }

  Move() : data(0) {}

  bool operator==(const Move &other) const { return data == other.data; }

  bool operator!=(const Move &other) const { return data != other.data; }

  std::string moveToUCIAlgebraic() {
    std::string result;
    result += squareName(getSrc());
    result += squareName(getDest());
    if (isPromotion()) {
      result += pieceToStringAlphLower(getPromotingPiece());
    }
    return result;
  }
};

#endif
