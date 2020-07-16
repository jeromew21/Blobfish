#ifndef MOVE_HPP
#define MOVE_HPP

#include <game/pieces.hpp>

namespace MoveTypeCode {
  const uint8_t Null = 0;
  const uint8_t Default = 1;
  const uint8_t CastleLong = 2;
  const uint8_t CastleShort = 3;
  const uint8_t DoublePawn = 4;
  const uint8_t EnPassant = 5;
  const uint8_t KPromotion = 6;
  const uint8_t BPromotion = 7;
  const uint8_t RPromotion = 8;
  const uint8_t QPromotion = 9;
}; // namespace MoveTypeCode

struct Move {
  uint16_t data;

  static Move NullMove() {
    static Move mv = Move(); // use "pawn" as null flag
    return mv;
  }

  bool notNull() {
    return getTypeCode() != MoveTypeCode::Null;
  }

  uint8_t getTypeCode() { return data & 15u; }

  PieceType getPromotingPiece() {
    uint8_t moveType = getTypeCode();
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

  bool isPromotion() {
    uint8_t tc = getTypeCode();
    return tc >= MoveTypeCode::KPromotion && tc <= MoveTypeCode::QPromotion;
  }

  u64 getSrc() {
    int s = data >> 10;
    return u64FromIndex(s);
  }

  u64 getDest() {
    int d = (data >> 4) & 63; // keep top 6 bits
    return u64FromIndex(d);
  }

  Move(int src0, int dest0, uint8_t typeCode) {
    data = (src0 << 10) | (dest0 << 4) | (typeCode & 15u);
  }

  Move() {
    data = 0;
  }

  bool operator==(const Move &other) const { return data == other.data; }
};

#endif
