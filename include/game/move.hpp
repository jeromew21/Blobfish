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
}; // namespace MoveTypeCode

struct Move {
  uint16_t data;

  static Move NullMove() { return Move(); }

  inline bool isNull() { return getTypeCode() == MoveTypeCode::Null; }

  inline bool notNull() { return getTypeCode() != MoveTypeCode::Null; }

  inline int getTypeCode() { return data & 15; }

  PieceType getPromotingPiece() {
    int moveType = getTypeCode();
    PieceType promotion = moveType - 5;
    return promotion;
  }

  PieceType getPromotingPiece(Color c) {
    PieceType promotion = getPromotingPiece();
    if (c == Black) {
      promotion += 6;
    }
    return promotion;
  }

  inline bool isPromotion() {
    int tc = getTypeCode();
    return tc >= MoveTypeCode::KPromotion && tc <= MoveTypeCode::QPromotion;
  }

  inline int getSrcIndex() { return data >> 10; }

  inline int getDestIndex() { return (data >> 4) & 63; }

  inline u64 getSrc() {
    int s = data >> 10;
    return u64FromIndex(s);
  }

  inline u64 getDest() {
    int d = (data >> 4) & 63; // keep top 6 bits
    return u64FromIndex(d);
  }

  Move(int src0, int dest0, int typeCode) {
    data = (src0 << 10) | (dest0 << 4) | (typeCode & 15);
  }

  Move() : data(0) {}

  bool operator==(const Move &other) const { return data == other.data; }
  
  bool operator!=(const Move &other) const { return data != other.data; }
};

#endif
