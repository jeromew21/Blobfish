#include <game/board.hpp>

int Board::material() { return material(White) - material(Black); }

int Board::material(Color color) {
  int result = 0;
  int start = pieceIndexFromColor(color);
  int end = start + 6;
  for (PieceType i = start; i < end; i++) {
    result += MATERIAL_TABLE[i] * hadd(bitboard[i]);
  }
  return result;
}

float Board::kingSafety(Color c) {
  //-5 for being next to one
  // pawn shield +10 bonus for 3 pawns and bottom row
  float score = 0.0;
  // u64 friendlies = board.occupancy(c);
  // u64 enemies = board.occupancy(flipColor(c));
  u64 kingBB = c == White ? bitboard[W_King] : bitboard[B_King];
  u64 pawns = c == White ? bitboard[W_Pawn] : bitboard[B_Pawn];
  int index = u64ToIndex(kingBB);
  int rookDir = c == White ? ROOK_UP : ROOK_DOWN;
  u64 backRank = getBackRank(c);
  int col = intToCol(index);
  int row = intToRow(index);
  bool isOnEdge = col % 7 == 0;

  // Keep pawns in front of king
  if (kingBB & backRank) {
    float pawnShieldScore = hadd(kingMoves(index) & pawns);
    if (isOnEdge) {
      pawnShieldScore /= 2.0f;
    } else {
      pawnShieldScore /= 3.0f;
    }
    score += pawnShieldScore * 3.0f;
  }

  // Penalize being on or next to open files
  float openFilesPenalty = 0.0f;
  if (!(rookMoves(index, rookDir) & pawns)) {
    openFilesPenalty += 2.0f;
  }
  if (col == 0) {
    int indexRight = intFromPair(row, col + 1);
    if (!(rookMoves(indexRight, rookDir) & pawns)) {
      openFilesPenalty += 1.0f;
    }
  } else if (col == 7) {
    int indexLeft = intFromPair(row, col - 1);
    if (!(rookMoves(indexLeft, rookDir) & pawns)) {
      openFilesPenalty += 1.0f;
    }
  } else {
    int indexLeft = intFromPair(row, col - 1);
    int indexRight = intFromPair(row, col + 1);
    if (!(rookMoves(indexLeft, rookDir) & pawns)) {
      openFilesPenalty += 1.0f;
    }
    if (!(rookMoves(indexRight, rookDir) & pawns)) {
      openFilesPenalty += 1.0f;
    }
  }
  score -= openFilesPenalty * 1.5f; // weight for open files

  return score;
}

float Board::tropism(u64 square, Color enemyColor) {
  static const int weights[6] = {0, 1, 1, 2, 3, 0};
  int row = u64ToRow(square);
  int col = u64ToRow(square);
  int sum = 0;
  std::array<int, 64> arr;
  int count;
  for (PieceType p = 1; p < 5; p++) {
    u64 bb = bitboard[p + 6 * enemyColor];
    bitscanAllInt(arr, bb, count);
    for (int i = 0; i < count; i++) {
      int index = arr[i];
      int eRow = intToRow(index);
      int eCol = intToCol(index);
      sum += (abs(eRow - row) + abs(eCol - col)) * weights[p];
    }
  }
  return -1.0f * sum;
}

int Board::mobility(Color c) { // Minor piece and rook mobility
  int result = 0;
  u64 friendlies = occupancy(c);
  u64 pieces = c == White
                   ? bitboard[W_Knight] | bitboard[W_Bishop] | bitboard[W_Rook]
                   : bitboard[B_Knight] | bitboard[B_Bishop] | bitboard[B_Rook];
  std::array<u64, 64> arr;
  int count;

  bitscanAll(arr, pieces, count);
  for (int i = 0; i < count; i++) {
    result += hadd(attackMap[u64ToIndex(arr[i])] & ~friendlies);
  }

  return result; // todo fix
}

std::string Board::vectorize() { // return a vector representation of the board
  std::string res = "";
  for (PieceType p = 0; p < 12; p++) {
    u64 x = 0;

    std::array<int, 64> arr0;
    int count;
    bitscanAllInt(arr0, bitboard[p], count);
    for (int i = 0; i < count; i++) {
      x |= attackMap[arr0[i]];
    }

    int arr[64];
    int i = 0;
    while (i < 64) {
      arr[i] = x & 1;
      x = x >> 1;
      i++;
    }
    for (int i = 7; i >= 0; i--) {
      for (int k = 0; k < 8; k++) {
        res += std::to_string(arr[i * 8 + k]);
      }
      res += "\n";
    }
  }
  return res;
}

void Board::perft(int depth, PerftCounter &pcounter) {
  if (depth == 0) {
    return;
  }

  std::vector<Move> moves = legalMoves();
  int s = moves.size();
  u64 occ = occupancy();
  for (int i = 0; i < s; i++) {
    Move mv = moves[i];
    if (depth == 1) {
      pcounter.nodes += 1;
      if (mv.isCastle()) {
        pcounter.castles += 1;
      }
      if (mv.isPromotion()) {
        pcounter.promotions += 1;
      }
      if (isCheckingMove(mv)) {
        pcounter.checks += 1;
      }
      if (mv.getDest() & occ) {
        pcounter.captures += 1;
      }
      if (mv.getTypeCode() == MoveTypeCode::EnPassant) {
        pcounter.ep += 1;
        pcounter.captures += 1;
      }
    }
    makeMove(mv);
    if (depth == 1) {
      auto s = status();
      if (s == BoardStatus::WhiteWin || s == BoardStatus::BlackWin) {
        pcounter.checkmates += 1;
      }
    }
    perft(depth - 1, pcounter);
    unmakeMove();
  }
}
