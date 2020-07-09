#ifndef MOVE_HPP
#define MOVE_HPP

#include <game/pieces.hpp>

enum class MoveType {
   Default = 0, EnPassant = 1, Promotion = 2, CastleLong = 3, CastleShort = 4, PawnDouble = 5, END = 6, Null = 7
};

struct Move
{
   PieceType mover;
   u64 src;
   u64 dest;
   MoveType moveType;
   PieceType destFormer; //if it was a capture, then this is the captured piece
   PieceType promotion; //piece to promote to

   static Move END() {
      //return Constructor(MoveType::END, 0, 0, 0, 0, 0);  
      //TODO: Optimize by always returning the same object
      static Move mv = Constructor(MoveType::END, 0, 0, 0, 0, 0);
      return mv;
   }

   static Move NullMove() {
      static Move mv = Constructor(MoveType::Null, 0, 0, 0, 0, 0);
      return mv;
   }

   static Move DefaultMove(PieceType mover, u64 src, u64 dest) {
         return Constructor(
            MoveType::Default, mover, src, dest, Empty, mover
         );
   }

   static Move SpecialMove(MoveType specialType, PieceType mover, u64 src, u64 dest) {
         return Constructor(
            specialType, mover, src, dest, Empty, mover
         );
   }

   static Move DoublePawnMove(PieceType mover, u64 src, u64 dest) {
         return Constructor(
            MoveType::PawnDouble, mover, src, dest, Empty, mover
         );
   }

   static Move PromotionMove(PieceType mover, u64 src, u64 dest, PieceType promotion) {
         return Constructor(
            MoveType::Promotion, mover, 
            src, dest, Empty, promotion
         );
   }

   static Move Constructor(
      MoveType moveType, 
      PieceType mover,
      u64 src, 
      u64 dest, 
      PieceType destFormer,
      PieceType promotion
   ) { //Return a value for a default move
      Move mv;
      mv.moveType = moveType;
      mv.src = src;
      mv.dest = dest;
      mv.mover = mover;
      mv.destFormer = destFormer;
      mv.promotion = promotion;
      return mv;
   }

   bool operator==(const Move &other) const {
      return (src == other.src && dest == other.dest) && (promotion == other.promotion);
   }

};

#endif
