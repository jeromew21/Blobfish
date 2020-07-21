#include <iostream>
#include <regex>
#include <string>

#include <game/board.hpp>

u64 BISHOP_MOVE_CACHE[64][4]; // outputs a bitboard w/ ray
u64 ROOK_MOVE_CACHE[64][4];

u64 KNIGHT_MOVE_CACHE[64];
u64 KING_MOVE_CACHE[64];
u64 PAWN_CAPTURE_CACHE[64][2];
u64 PAWN_MOVE_CACHE[64][2];
u64 PAWN_DOUBLE_CACHE[64][2];

u64 ONE_ADJACENT_CACHE[64];

u64 CASTLE_LONG_SQUARES[2];
u64 CASTLE_SHORT_SQUARES[2];
u64 CASTLE_LONG_KING_SLIDE[2];
u64 CASTLE_SHORT_KING_SLIDE[2];
u64 CASTLE_LONG_KING_DEST[2];
u64 CASTLE_SHORT_KING_DEST[2];
u64 CASTLE_LONG_ROOK_DEST[2];
u64 CASTLE_SHORT_ROOK_DEST[2];

PieceSquareTable PIECE_SQUARE_TABLE[12][2]; //one for earlygame, one for endgame

u64 ZOBRIST_HASHES[781];

int SIDE_TO_MOVE_HASH_POS = 64 * 12;
int W_LONG_HASH_POS = SIDE_TO_MOVE_HASH_POS + 1;
int W_SHORT_HASH_POS = SIDE_TO_MOVE_HASH_POS + 2;
int B_LONG_HASH_POS = SIDE_TO_MOVE_HASH_POS + 3;
int B_SHORT_HASH_POS = SIDE_TO_MOVE_HASH_POS + 4;
int EP_HASH_POS = SIDE_TO_MOVE_HASH_POS + 5;

PieceType CLASSICAL_BOARD[] = { // 64 squares
    W_Rook, W_Knight, W_Bishop, W_Queen, W_King, W_Bishop, W_Knight, W_Rook,
    W_Pawn, W_Pawn,   W_Pawn,   W_Pawn,  W_Pawn, W_Pawn,   W_Pawn,   W_Pawn,
    Empty,  Empty,    Empty,    Empty,   Empty,  Empty,    Empty,    Empty,
    Empty,  Empty,    Empty,    Empty,   Empty,  Empty,    Empty,    Empty,
    Empty,  Empty,    Empty,    Empty,   Empty,  Empty,    Empty,    Empty,
    Empty,  Empty,    Empty,    Empty,   Empty,  Empty,    Empty,    Empty,
    B_Pawn, B_Pawn,   B_Pawn,   B_Pawn,  B_Pawn, B_Pawn,   B_Pawn,   B_Pawn,
    B_Rook, B_Knight, B_Bishop, B_Queen, B_King, B_Bishop, B_Knight, B_Rook};

void initializeZobrist() {
  for (int i = 0; i < 781; i++) {
    u64 hashval = 0;
    u64 bitmover = 1;
    for (int k = 0; k < 64; k++) {
      int randNum = rand100();
      if (randNum > 50) {
        hashval |= bitmover;
      }
      bitmover <<= 1;
    }
    ZOBRIST_HASHES[i] = hashval;
  }
  debugLog("Initialized zobrist hashes");
}

u64 kingMoves(int i) {
  return KING_MOVE_CACHE[i];
}

std::string moveToUCIAlgebraic(Move mv) {
  std::string result;
  result += squareName(mv.getSrc());
  result += squareName(mv.getDest());
  if (mv.isPromotion()) {
    result += pieceToStringAlphLower(mv.getPromotingPiece());
  }
  return result;
}

void populateMoveCache() {
  const int ROOK_MOVES[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
  const int BISHOP_MOVES[4][2] = {{1, -1}, {1, 1}, {-1, 1}, {-1, -1}};

  const int KNIGHT_MOVES[8][2] = {
      {1, 2}, {1, -2}, {-1, 2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}, {-2, -1},
  };
  const int PAWN_CAPTURES[2] = {-1, 1};

  CASTLE_LONG_SQUARES[White] =
      u64FromIndex(1) | u64FromIndex(2) | u64FromIndex(3);
  CASTLE_LONG_SQUARES[Black] =
      u64FromIndex(57) | u64FromIndex(58) | u64FromIndex(59);
  CASTLE_SHORT_SQUARES[White] = u64FromIndex(5) | u64FromIndex(6);
  CASTLE_SHORT_SQUARES[Black] = u64FromIndex(61) | u64FromIndex(62);

  CASTLE_LONG_KING_SLIDE[White] = u64FromIndex(2) | u64FromIndex(3);
  CASTLE_LONG_KING_SLIDE[Black] = u64FromIndex(58) | u64FromIndex(59);
  CASTLE_SHORT_KING_SLIDE[White] = u64FromIndex(5) | u64FromIndex(6);
  CASTLE_SHORT_KING_SLIDE[Black] = u64FromIndex(61) | u64FromIndex(62);

  CASTLE_LONG_KING_DEST[White] = u64FromIndex(2);
  CASTLE_LONG_KING_DEST[Black] = u64FromIndex(58);
  CASTLE_SHORT_KING_DEST[White] = u64FromIndex(6);
  CASTLE_SHORT_KING_DEST[Black] = u64FromIndex(62);

  CASTLE_LONG_ROOK_DEST[White] = u64FromIndex(3);
  CASTLE_LONG_ROOK_DEST[Black] = u64FromIndex(59);
  CASTLE_SHORT_ROOK_DEST[White] = u64FromIndex(5);
  CASTLE_SHORT_ROOK_DEST[Black] = u64FromIndex(61);

  for (int i = 0; i < 64; i++) {
    u64 bitPos = u64FromIndex(i);
    int y0 = u64ToRow(bitPos);
    int x0 = u64ToCol(bitPos);

    // SPECIAL
    u64 pawnAdj = 0;
    if (inBounds(y0, x0 + 1)) {
      pawnAdj |= u64FromPair(y0, x0 + 1);
    }
    if (inBounds(y0, x0 - 1)) {
      pawnAdj |= u64FromPair(y0, x0 - 1);
    }
    ONE_ADJACENT_CACHE[i] = pawnAdj;

    // Kings
    u64 bitmap = 0;
    for (int dir = 0; dir < 4; dir++) {
      int y = y0 + ROOK_MOVES[dir][0];
      int x = x0 + ROOK_MOVES[dir][1];
      if (inBounds(y, x)) {
        bitmap |= u64FromPair(y, x);
      }
      y = y0 + BISHOP_MOVES[dir][0];
      x = x0 + BISHOP_MOVES[dir][1];
      if (inBounds(y, x)) {
        bitmap |= u64FromPair(y, x);
      }
    }
    KING_MOVE_CACHE[i] = bitmap;

    // White Pawns
    bitmap = 0;
    for (int dir = 0; dir < 2; dir++) {
      int y = y0 + 1;
      int x = x0 + PAWN_CAPTURES[dir];
      if (inBounds(y, x)) {
        bitmap |= u64FromPair(y, x);
      }
    }
    PAWN_CAPTURE_CACHE[i][White] = bitmap;
    bitmap = 0;
    if (inBounds(y0 + 1, x0)) {
      bitmap |= u64FromPair(y0 + 1, x0);
    }
    PAWN_MOVE_CACHE[i][White] = bitmap;

    bitmap = 0;
    if (y0 == 1) {
      bitmap |= u64FromPair(y0 + 2, x0);
    }
    PAWN_DOUBLE_CACHE[i][White] = bitmap;

    // Black Pawns
    bitmap = 0;
    for (int dir = 0; dir < 2; dir++) {
      int y = y0 - 1;
      int x = x0 + PAWN_CAPTURES[dir];
      if (inBounds(y, x)) {
        bitmap |= u64FromPair(y, x);
      }
    }
    PAWN_CAPTURE_CACHE[i][Black] = bitmap;
    bitmap = 0;
    if (inBounds(y0 - 1, x0)) {
      bitmap |= u64FromPair(y0 - 1, x0);
    }
    PAWN_MOVE_CACHE[i][Black] = bitmap;

    bitmap = 0;
    if (y0 == 6) {
      bitmap |= u64FromPair(y0 - 2, x0);
    }
    PAWN_DOUBLE_CACHE[i][Black] = bitmap;

    // Knights
    bitmap = 0;
    for (int dir = 0; dir < 8; dir++) {
      int y = y0 + KNIGHT_MOVES[dir][0];
      int x = x0 + KNIGHT_MOVES[dir][1];
      if (inBounds(y, x)) {
        bitmap |= u64FromPair(y, x);
      }
    }
    KNIGHT_MOVE_CACHE[i] = bitmap;

    // Bishops
    for (int dir = 0; dir < 4; dir++) {
      u64 bitmap = 0;
      int dy = BISHOP_MOVES[dir][0];
      int dx = BISHOP_MOVES[dir][1];
      int y = y0 + dy;
      int x = x0 + dx;
      while (inBounds(y, x)) {
        bitmap |= u64FromPair(y, x);
        y += dy;
        x += dx;
      }
      BISHOP_MOVE_CACHE[i][dir] = bitmap;
    }

    // Rooks
    for (int dir = 0; dir < 4; dir++) {
      u64 bitmap = 0;
      int dy = ROOK_MOVES[dir][0];
      int dx = ROOK_MOVES[dir][1];
      int y = y0 + dy;
      int x = x0 + dx;
      while (inBounds(y, x)) {
        bitmap |= u64FromPair(y, x);
        y += dy;
        x += dx;
      }
      ROOK_MOVE_CACHE[i][dir] = bitmap;
    }
  }
  debugLog("Initialized move cache");

  //init piece squares
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      int index = intFromPair(row, col);
      //weight pawns lower and kings higher
      float rr = row/14.0;
      float r7 = ((float)(7-row))/14.0;
      PIECE_SQUARE_TABLE[W_Pawn][0].set(index, rr);
      PIECE_SQUARE_TABLE[W_Pawn][1].set(index, rr);
      PIECE_SQUARE_TABLE[B_Pawn][0].set(index, r7);
      PIECE_SQUARE_TABLE[B_Pawn][1].set(index, r7);

      r7 = 7 - distToClosestCorner(row, col);
      rr = distToClosestCorner(row, col);
      r7 /= 2.5;
      rr /= 7.0;
      PIECE_SQUARE_TABLE[W_King][0].set(index, r7);
      PIECE_SQUARE_TABLE[B_King][0].set(index, r7);
      PIECE_SQUARE_TABLE[W_King][1].set(index, rr);
      PIECE_SQUARE_TABLE[B_King][1].set(index, rr);
      
      float nmoves = hadd(KNIGHT_MOVE_CACHE[index]);
      nmoves /= 8.0;
      PIECE_SQUARE_TABLE[W_Knight][0].set(index, nmoves);
      PIECE_SQUARE_TABLE[W_Knight][1].set(index, nmoves);
      PIECE_SQUARE_TABLE[B_Knight][0].set(index, nmoves);
      PIECE_SQUARE_TABLE[B_Knight][1].set(index, nmoves);

      float bmoves = 0;
      float rmoves = 0;
      for (int d = 0; d < 4; d++) {
        bmoves += hadd(BISHOP_MOVE_CACHE[index][d]);
        rmoves += hadd(ROOK_MOVE_CACHE[index][d]);
      }
      float qmoves = bmoves+rmoves;
      bmoves /= 21.0;
      rmoves /= 14.0;
      qmoves /= 54.0;
      PIECE_SQUARE_TABLE[W_Bishop][0].set(index, bmoves);
      PIECE_SQUARE_TABLE[W_Bishop][1].set(index, bmoves);
      PIECE_SQUARE_TABLE[B_Bishop][0].set(index, bmoves);
      PIECE_SQUARE_TABLE[B_Bishop][1].set(index, bmoves);
      PIECE_SQUARE_TABLE[W_Rook][0].set(index, rmoves);
      PIECE_SQUARE_TABLE[W_Rook][1].set(index, rmoves);
      PIECE_SQUARE_TABLE[B_Rook][0].set(index, rmoves);
      PIECE_SQUARE_TABLE[B_Rook][1].set(index, rmoves);
      PIECE_SQUARE_TABLE[W_Queen][0].set(index, qmoves);
      PIECE_SQUARE_TABLE[W_Queen][1].set(index, qmoves);
      PIECE_SQUARE_TABLE[B_Queen][0].set(index, qmoves);
      PIECE_SQUARE_TABLE[B_Queen][1].set(index, qmoves);
    }
  }
}

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

void Board::_generatePseudoLegal() {
  // generate attack-defend sets
  for (int i = 0; i < 64; i++) {
    attackMap[i] = 0;
    defendMap[i] = 0;
  }
  u64 occ = occupancy();

  // for each piece: drop in to squares attacked
  std::array<u64, 64> arr;
  int count;

  bitscanAll(arr, bitboard[W_Pawn], count);
  for (int i = 0; i < count; i++) {
    u64 pos = arr[i]; // position of piece
    int posIndex = u64ToIndex(pos);
    u64 attacks = PAWN_CAPTURE_CACHE[posIndex][White];
    attackMap[posIndex] |= attacks;
  }
  bitscanAll(arr, bitboard[B_Pawn], count);
  for (int i = 0; i < count; i++) {
    u64 pos = arr[i]; // position of piece
    int posIndex = u64ToIndex(pos);
    u64 attacks = PAWN_CAPTURE_CACHE[posIndex][Black];
    attackMap[posIndex] |= attacks;
  }

  bitscanAll(arr, bitboard[W_Knight] | bitboard[B_Knight], count);
  for (int i = 0; i < count; i++) {
    u64 pos = arr[i]; // position of piece
    int posIndex = u64ToIndex(pos);
    u64 attacks = KNIGHT_MOVE_CACHE[posIndex];
    attackMap[posIndex] |= attacks;
  }

  bitscanAll(arr, bitboard[W_King] | bitboard[B_King], count);
  for (int i = 0; i < count; i++) {
    u64 pos = arr[i]; // position of piece
    int posIndex = u64ToIndex(pos);
    u64 attacks = KING_MOVE_CACHE[posIndex];
    attackMap[posIndex] |= attacks;
  }

  bitscanAll(arr,
             bitboard[W_Bishop] | bitboard[B_Bishop] | bitboard[W_Queen] |
                 bitboard[B_Queen],
             count);
  for (int i = 0; i < count; i++) {
    u64 pos = arr[i]; // position of piece
    int posIndex = u64ToIndex(pos);
    u64 attacks = _bishopAttacks(pos, occ);
    attackMap[posIndex] |= attacks;
  }

  bitscanAll(arr,
             bitboard[W_Rook] | bitboard[B_Rook] | bitboard[W_Queen] |
                 bitboard[B_Queen],
             count);
  for (int i = 0; i < count; i++) {
    u64 pos = arr[i]; // position of piece
    int posIndex = u64ToIndex(pos);
    u64 attacks = _rookAttacks(pos, occ);
    attackMap[posIndex] |= attacks;
  }

  // load defend map
  for (int i = 0; i < 64; i++) {
    u64 attacked = attackMap[i];
    u64 attackerSq = u64FromIndex(i);
    // for each attacked:
    bitscanAll(arr, attacked, count);
    for (int k = 0; k < count; k++) {
      u64 defenderSq = arr[k];
      int defenderIndex = u64ToIndex(defenderSq);
      defendMap[defenderIndex] |= attackerSq;
    }
  }

  _pseudoStack.push_back(PseudoLegalData(attackMap, defendMap));
}

u64 Board::_isUnderAttack(u64 target) { 
  u64 result = 0;
  std::array<int, 64> arr;
  int count;
  bitscanAllInt(arr, target, count);
  for (int i = 0; i < count; i++) {
    result |= defendMap[arr[i]];
  }
  return result;
}

u64 Board::_isUnderAttack(u64 target, Color byWho) {
  return _isUnderAttack(target) & occupancy(byWho);
}

bool Board::isCheck() {
  Color color = turn();
  u64 kingBB = color == White ? bitboard[W_King] : bitboard[B_King];
  return _isUnderAttack(kingBB, flipColor(color));
}

std::vector<Move> Board::produceUncheckMoves() {
  // todo make this work
  std::vector<Move> result;
  LazyMovegen movegen(occupancy(turn()), attackMap);
  std::vector<Move> sbuffer;
  bool hasGenSpecial = false;
  Move mv = nextMove(movegen, sbuffer, hasGenSpecial);
  if (mv.notNull()) {
    result.push_back(mv);
  }
  return result;
}

PieceType Board::pieceAt(u64 space, Color c) {
  PieceType s = pieceIndexFromColor(c);
  // temporary solution
  for (PieceType i = s; i < s + 6; i++) {
    if (space & bitboard[i]) {
      return i;
    }
  }
  return Empty;
}

Board::Board() { reset(); }

PieceType Board::pieceAt(u64 space) {
  // temporary solution
  for (PieceType i = 0; i < Empty; i++) {
    if (space & bitboard[i]) {
      return i;
    }
  }
  return Empty;
}

Move Board::moveFromAlgebraic(const std::string &alg) {
  // shortcut way
  auto moves = legalMoves();
  for (Move &mv : moves) {
    if (moveToUCIAlgebraic(mv).compare(alg) == 0 ||
        (moveToExtAlgebraic(mv).compare(alg) == 0 ||
         moveToAlgebraic(mv).compare(alg) == 0)) {
      return mv;
    }
  }
  return Move::NullMove();
}

std::string Board::moveToExtAlgebraic(Move mv) {
  if (mv.getTypeCode() == MoveTypeCode::CastleLong) {
    return "O-O-O";
  } else if (mv.getTypeCode() == MoveTypeCode::CastleShort) {
    return "O-O";
  }
  std::string result;
  PieceType mover = pieceAt(mv.getSrc());
  if (mover != W_Pawn && mover != B_Pawn) {
    result += pieceToStringAlph(mover);
  }
  result += squareName(mv.getSrc());
  if (mv.getDest() & occupancy() ||
      mv.getTypeCode() == MoveTypeCode::EnPassant) {
    result += "x";
  } else {
    result += "-";
  }
  result += squareName(mv.getDest());
  if (mv.isPromotion()) {
    result += pieceToStringAlph(mv.getPromotingPiece());
  }
  return result;
}

bool Board::canUndo() { return stack.canPop(); }

std::string Board::moveToAlgebraic(Move mv) {
  if (mv.getTypeCode() == MoveTypeCode::CastleLong) {
    return "O-O-O";
  } else if (mv.getTypeCode() == MoveTypeCode::CastleShort) {
    return "O-O";
  }
  std::string result;
  PieceType mover = pieceAt(mv.getSrc());

  if (mover != W_Pawn && mover != B_Pawn) {
    result += pieceToStringAlph(mover);
  }
  // DISAMBIGUATE
  if ((mover == W_Rook || mover == B_Rook) ||
      (mover == W_Knight || mover == B_Knight) ||
      (mover == W_Queen || mover == B_Queen) ||
      (mover == W_Bishop || mover == W_Bishop)) {
    auto moves = legalMoves();
    std::string colDisambig = "";
    std::string rowDisambig = "";
    int overlapCount = 0;
    for (Move &testMove : moves) {
      PieceType testmover = pieceAt(mv.getSrc());

      if (mover == testmover && pieceAt(mv.getDest()) == testmover) {
        // if col is different, add col
        if (u64ToCol(mv.getSrc()) != u64ToCol(testMove.getSrc())) {
          colDisambig = FILE_NAMES[u64ToCol(mv.getSrc())];
        }
        // if row is different, add row
        if (u64ToRow(mv.getSrc()) != u64ToRow(testMove.getSrc())) {
          rowDisambig = RANK_NAMES[u64ToRow(mv.getSrc())];
        }
        overlapCount += 1;
      }
    }
    if (overlapCount > 2) {
      result += colDisambig + rowDisambig;
    } else {
      if (colDisambig.length() > 0) {
        result += colDisambig;
      } else if (rowDisambig.length() > 0) {
        result += rowDisambig;
      }
    }
  }
  if (mv.getDest() & occupancy() ||
      mv.getTypeCode() == MoveTypeCode::EnPassant) {
    if (mover == W_Pawn || mover == B_Pawn) {
      result += FILE_NAMES[u64ToCol(mv.getSrc())];
    }
    result += "x";
  }
  result += squareName(mv.getDest());
  if (mv.isPromotion()) {
    result += "=" + pieceToStringAlph(mv.getPromotingPiece());
  }
  return result;
}

BoardStatus Board::status() {
  if (_status == BoardStatus::NotCalculated) {
    if (boardState[HAS_REPEATED_INDEX]) {
      _status = BoardStatus::Draw;
      return _status;
    }
    // calculate

    // TODO: Fix insuff material
    if (hadd(bitboard[W_Pawn] | bitboard[B_Pawn] | bitboard[B_Queen] |
              bitboard[W_Queen] | bitboard[W_Rook] | bitboard[B_Rook]) == 0) {
      // if either side has at least two minor pieces and one bishop
      // not a draw
      if (!((hadd(bitboard[W_Bishop] | bitboard[W_Knight]) >= 2 &&
             hadd(bitboard[W_Bishop]) > 0) ||
            (hadd(bitboard[B_Bishop] | bitboard[B_Knight]) >= 2 &&
             hadd(bitboard[B_Bishop]) > 0))) {
        _status = BoardStatus::Draw;
        return _status;
      }
    }

    auto movelist = produceUncheckMoves();
    if (movelist.size() == 0) {
      if (isCheck()) {
        _status =
            turn() == White ? BoardStatus::BlackWin : BoardStatus::WhiteWin;
      } else {
        _status = BoardStatus::Stalemate;
      }
      return _status;
    }
    _status = BoardStatus::Playing;
    return _status;
  } else {
    return _status;
  }
}

void Board::_switchTurn() { _switchTurn(flipColor(turn())); }

void Board::_switchTurn(Color t) {
  // if turn is black, then xor in
  // else, xor out
  u64 hash = ZOBRIST_HASHES[64 * 12];
  if (t != turn()) {
    _zobristHash ^= hash;
  }
  boardState[TURN_INDEX] = t;
}

u64 Board::zobrist() { return _zobristHash; }

void Board::_removePiece(PieceType p, u64 location) {
  // location may be empty; if so, do nothing
  // XOR out hash
  if (location & bitboard[p]) {
    int ind = u64ToIndex(location);
    u64 hash = ZOBRIST_HASHES[64 * p + ind];
    _zobristHash ^= hash;
    pieceScoreEarlyGame[p] -= PIECE_SQUARE_TABLE[p][0].at(ind);
    pieceScoreLateGame[p] -= PIECE_SQUARE_TABLE[p][1].at(ind);
  } else {
    std::cout << pieceToString(p) << "\n";
    dump64(location);
    dump64(bitboard[p]);
    dump(true);
    debugLog("bad piecerem");
    throw;
  }
  bitboard[p] &= ~location;
}

void Board::_addPiece(PieceType p, u64 location) {
  // assume that location is empty
  // XOR in hash
  if (location & bitboard[p]) {
    std::cout << pieceToString(p) << "\n";
    dump64(location);
    dump64(bitboard[p]);
    dump(true);
    debugLog("bad pieceadd");
    throw;
  } else {
    int ind = u64ToIndex(location);
    u64 hash = ZOBRIST_HASHES[64 * p + ind];
    _zobristHash ^= hash;
    pieceScoreEarlyGame[p] += PIECE_SQUARE_TABLE[p][0].at(ind);
    pieceScoreLateGame[p] += PIECE_SQUARE_TABLE[p][1].at(ind);
  }
  bitboard[p] |= location;
}

void Board::makeMove(Move mv) {
  _status = BoardStatus::NotCalculated;

  // copy old data and move onto stack
  stack.push(boardState, mv, zobrist());

  int moveType = mv.getTypeCode();

  boardState[LAST_MOVED_INDEX] = Empty;
  boardState[LAST_CAPTURED_INDEX] = Empty;

  if (moveType != MoveTypeCode::Null) {
    Color c = turn();
    u64 src = mv.getSrc();
    u64 dest = mv.getDest();
    PieceType mover = pieceAt(src);
    PieceType destFormer = pieceAt(dest);

    boardState[LAST_MOVED_INDEX] = mover;

    // mask out mover
    _removePiece(mover, src);

    if (moveType == MoveTypeCode::EnPassant) {
      if (mover == W_Pawn) {
        u64 capturedPawn = PAWN_MOVE_CACHE[u64ToIndex(dest)][Black];
        _removePiece(B_Pawn, capturedPawn);
        boardState[LAST_CAPTURED_INDEX] = B_Pawn;
      } else {
        u64 capturedPawn = PAWN_MOVE_CACHE[u64ToIndex(dest)][White];
        _removePiece(W_Pawn, capturedPawn);
        boardState[LAST_CAPTURED_INDEX] = W_Pawn;
      }
    } else if (destFormer != Empty) {
      _removePiece(destFormer, dest);
      boardState[LAST_CAPTURED_INDEX] = destFormer;
    } else {
      boardState[LAST_CAPTURED_INDEX] = Empty; // wipe it
    }

    // move to new location
    if (mv.isPromotion()) {
      _addPiece(mv.getPromotingPiece(c), dest);
    } else {
      _addPiece(mover, dest);
    }

    if (moveType == MoveTypeCode::CastleLong) {
      if (mover == W_King) {
        _removePiece(W_Rook, rookStartingPositions[White][0]);
        _addPiece(W_Rook, CASTLE_LONG_ROOK_DEST[White]);
      } else {
        _removePiece(B_Rook, rookStartingPositions[Black][0]);
        _addPiece(B_Rook, CASTLE_LONG_ROOK_DEST[Black]);
      }
    } else if (moveType == MoveTypeCode::CastleShort) {
      if (mover == W_King) {
        _removePiece(W_Rook, rookStartingPositions[White][1]);
        _addPiece(W_Rook, CASTLE_SHORT_ROOK_DEST[White]);
      } else {
        _removePiece(B_Rook, rookStartingPositions[Black][1]);
        _addPiece(B_Rook, CASTLE_SHORT_ROOK_DEST[Black]);
      }
    }

    // revoke castling rights
    if (mover == W_King) {
      _setCastlingPrivileges(White, 0, 0);
    } else if (mover == B_King) {
      _setCastlingPrivileges(Black, 0, 0);
    }
    if ((mover == B_Rook || mover == W_Rook) ||
        (destFormer == B_Rook || destFormer == W_Rook)) {
      u64 targets = src | dest;
      if (targets & rookStartingPositions[White][0]) {
        _setCastlingPrivileges(White, 0, boardState[W_SHORT_INDEX]);
      }
      if (targets & rookStartingPositions[White][1]) {
        _setCastlingPrivileges(White, boardState[W_LONG_INDEX], 0);
      }
      if (targets & rookStartingPositions[Black][0]) {
        _setCastlingPrivileges(Black, 0, boardState[B_SHORT_INDEX]);
      }
      if (targets & rookStartingPositions[Black][1]) {
        _setCastlingPrivileges(Black, boardState[B_LONG_INDEX], 0);
      }
    }

    // handle en passant
    if (moveType == MoveTypeCode::DoublePawn) {
      // see if adjacent squares have pawns
      if (mover == W_Pawn) {
        int destIndex = u64ToIndex(dest);
        if (bitboard[B_Pawn] & ONE_ADJACENT_CACHE[destIndex]) {
          _setEpSquare(u64ToIndex(PAWN_MOVE_CACHE[destIndex][Black]));
        } else {
          _setEpSquare(-1);
        }
      } else {
        int destIndex = u64ToIndex(dest);
        if (bitboard[W_Pawn] & ONE_ADJACENT_CACHE[destIndex]) {
          _setEpSquare(u64ToIndex(PAWN_MOVE_CACHE[destIndex][White]));
        } else {
          _setEpSquare(-1);
        }
      }
    } else {
      // all other moves: en passant square is nulled
      _setEpSquare(-1);
    }
  } else {
    _setEpSquare(-1);
  }

  _switchTurn();

  _generatePseudoLegal();

  if (!boardState[HAS_REPEATED_INDEX]) {
    int counter = 0;
    for (int i = stack.getIndex() - 2; i >= 0; i -= 2) {
      BoardStateNode &node = stack.peekNodeAt(i);
      if (node.hash == zobrist()) {
        counter++;
      }
      if (node.data[LAST_MOVED_INDEX] % 6 == 0 ||
          node.data[LAST_CAPTURED_INDEX] != Empty) {
        break;
      }
    }
    if (counter >= 2) {
      boardState[HAS_REPEATED_INDEX] = 1;
    }
  }
}

void Board::unmakeMove() {
  // state changer

  BoardStateNode &node = stack.peek();
  Move &mv = node.mv;
  int moveType = mv.getTypeCode();

  if (moveType != MoveTypeCode::Null) {
    u64 src = mv.getSrc();
    u64 dest = mv.getDest();
    int moveType = mv.getTypeCode();
    PieceType mover = boardState[LAST_MOVED_INDEX];
    PieceType destFormer = boardState[LAST_CAPTURED_INDEX];

    if (moveType == MoveTypeCode::CastleLong ||
        moveType == MoveTypeCode::CastleShort) {
      if (mover == W_King) {
        if (moveType == MoveTypeCode::CastleLong) {
          _removePiece(W_Rook, CASTLE_LONG_ROOK_DEST[White]);
          _addPiece(W_Rook, rookStartingPositions[White][0]);
        } else {
          _removePiece(W_Rook, CASTLE_SHORT_ROOK_DEST[White]);
          _addPiece(W_Rook, rookStartingPositions[White][1]);
        }
      } else {
        if (moveType == MoveTypeCode::CastleLong) {
          _removePiece(B_Rook, CASTLE_LONG_ROOK_DEST[Black]);
          _addPiece(B_Rook, rookStartingPositions[Black][0]);
        } else {
          _removePiece(B_Rook, CASTLE_SHORT_ROOK_DEST[Black]);
          _addPiece(B_Rook, rookStartingPositions[Black][1]);
        }
      }
    }

    // move piece to old src
    _addPiece(mover, src);

    if (mv.isPromotion()) {
      _removePiece(mv.getPromotingPiece(flipColor(turn())), dest);
    } else {
      _removePiece(mover, dest);
    }

    // restore dest
    if (moveType ==
        MoveTypeCode::EnPassant) { // instead of restoring at capture
                                   // location, restore one above
      if (mover == W_Pawn) {
        _addPiece(B_Pawn, PAWN_MOVE_CACHE[u64ToIndex(dest)][Black]);
      } else {
        _addPiece(W_Pawn, PAWN_MOVE_CACHE[u64ToIndex(dest)][White]);
      }
    } else if (destFormer != Empty) {
      _addPiece(destFormer, dest); // restore to capture location
    }
  }

  _switchTurn(node.data[TURN_INDEX]);
  _setEpSquare(node.data[EN_PASSANT_INDEX]);
  _setCastlingPrivileges(White, node.data[W_LONG_INDEX],
                         node.data[W_SHORT_INDEX]);
  _setCastlingPrivileges(Black, node.data[B_LONG_INDEX],
                         node.data[B_SHORT_INDEX]);
  boardState[LAST_MOVED_INDEX] = node.data[LAST_MOVED_INDEX];
  boardState[LAST_CAPTURED_INDEX] = node.data[LAST_CAPTURED_INDEX];
  boardState[HAS_REPEATED_INDEX] = node.data[HAS_REPEATED_INDEX];
  _status = BoardStatus::NotCalculated;

  stack.pop();

  _pseudoStack.pop_back(); // pop current

  PseudoLegalData pdata = _pseudoStack.back(); // obtain old
  attackMap = pdata.aMap;
  defendMap = pdata.dMap;
}

void Board::_setCastlingPrivileges(Color color, int cLong, int cShort) {
  // If different: then xor in
  if (color == White) {
    if (cLong != boardState[W_LONG_INDEX]) {
      u64 hash = ZOBRIST_HASHES[W_LONG_HASH_POS];
      _zobristHash ^= hash;
    }
    if (cShort != boardState[W_SHORT_INDEX]) {
      u64 hash = ZOBRIST_HASHES[W_SHORT_HASH_POS];
      _zobristHash ^= hash;
    }
  } else {
    if (cLong != boardState[B_LONG_INDEX]) {
      u64 hash = ZOBRIST_HASHES[B_LONG_HASH_POS];
      _zobristHash ^= hash;
    }
    if (cShort != boardState[B_SHORT_INDEX]) {
      u64 hash = ZOBRIST_HASHES[B_SHORT_HASH_POS];
      _zobristHash ^= hash;
    }
  }

  if (color == White) {
    boardState[W_LONG_INDEX] = cLong;
    boardState[W_SHORT_INDEX] = cShort;
  } else {
    boardState[B_LONG_INDEX] = cLong;
    boardState[B_SHORT_INDEX] = cShort;
  }
  // INC UPDATE
}

void Board::_setEpSquare(int sq) {
  int currentSq = boardState[EN_PASSANT_INDEX];
  if (sq == currentSq) {
    return; // do nothing
  }
  if (currentSq != -1) {
    // there is an en passant square already
    // we want to remove it
    int currentRow = u64ToRow(currentSq);
    u64 hash = ZOBRIST_HASHES[currentRow];
    _zobristHash ^= hash;
  }
  if (sq != -1) {
    // we are adding a new one
    int newRow = u64ToRow(sq);
    u64 hash = ZOBRIST_HASHES[newRow];
    _zobristHash ^= hash;
  }
  boardState[EN_PASSANT_INDEX] = sq;
}

void Board::dump() { dump(false); }

std::string Board::fen() {
  std::string result = "";
  for (int i = 7; i >= 0; i--) {
    if (i < 7) {
      result += "/";
    }
    int ecounter = 0;
    for (int k = 0; k < 8; k++) {
      PieceType piece = pieceAt(u64FromPair(i, k));
      if (piece == Empty) {
        ecounter++;
      } else {
        if (ecounter > 0) {
          result += std::to_string(ecounter);
        }
        result += pieceToStringFen(piece);
        ecounter = 0;
      }
    }
    if (ecounter > 0) {
      result += std::to_string(ecounter);
    }
  }
  result += " ";
  result += turn() == White ? "w" : "b";
  if (boardState[W_SHORT_INDEX] || boardState[W_LONG_INDEX] ||
      boardState[B_SHORT_INDEX] || boardState[B_LONG_INDEX]) {
    result += " ";
    result += boardState[W_SHORT_INDEX] ? "K" : "";
    result += boardState[W_LONG_INDEX] ? "Q" : "";
    result += boardState[B_SHORT_INDEX] ? "k" : "";
    result += boardState[B_LONG_INDEX] ? "q" : "";
  } else {
    result += " -";
  }
  result += " ";
  if (boardState[EN_PASSANT_INDEX] >= 0) {
    result += squareName(boardState[EN_PASSANT_INDEX]);
  } else {
    result += "-";
  }
  result += " 0 ";                                // clock
  result += std::to_string(stack.getIndex() + 1); // num moves
  return result;
}

void Board::dump(bool debug) {
  for (int i = 7; i >= 0; i--) {
    for (int k = 0; k < 8; k++) {
      std::cout << pieceToString(pieceAt(u64FromPair(i, k)));
    }
    std::cout << "\n";
  }
  if (debug) {
    auto moves = legalMoves();

    std::cout << "Turn: " << colorToString(turn()) << "\n";
    std::cout << "Game Status: " << statusToString(status(), false) << "\n";
    std::cout << "Is Check: " << yesorno(isCheck()) << "\n";
    std::cout << "White can castle: (" << yesorno(boardState[W_LONG_INDEX])
              << ", " << yesorno(boardState[W_SHORT_INDEX]) << ")\n";
    std::cout << "Black can castle: (" << yesorno(boardState[B_LONG_INDEX])
              << ", " << yesorno(boardState[B_SHORT_INDEX]) << ")\n";
    std::cout << "EP square: ";
    if (boardState[EN_PASSANT_INDEX] >= 0) {
      std::cout << squareName(boardState[EN_PASSANT_INDEX]);
    } else {
      std::cout << "none";
    }
    std::cout << "\nLast Mover:";
    if (boardState[LAST_MOVED_INDEX] != Empty) {
      std::cout << pieceToString(boardState[LAST_MOVED_INDEX]);
    }
    std::cout << "\nLast Captured:";
    if (boardState[LAST_CAPTURED_INDEX] != Empty) {
      std::cout << pieceToString(boardState[LAST_CAPTURED_INDEX]);
    }
    std::cout << "\nLegal: ";
    for (Move &mv : moves) {
      std::cout << moveToUCIAlgebraic(mv) << " ";
      if (mv.getDest() & occupancy()) {
        std::cout << ": " << see(mv) << " ";
      }
    }
    std::cout << "\nOut-Of-Check: ";
    for (Move &mv : produceUncheckMoves()) {
      std::cout << moveToAlgebraic(mv) << " ";
    }
    std::cout << "\nIs repetition: ";
    std::cout << yesorno(boardState[HAS_REPEATED_INDEX]);
    std::cout << "\nMoves made: ";
    for (int i = 0; i < (int)stack.getIndex(); i++) {
      Move mv = stack.peekAt(i);
      std::cout << moveToUCIAlgebraic(mv) << " ";
      /*std::cout << "(" << (int)mv.getTypeCode() << ")"
                << " ";*/
    }
    std::cout << "\nPiece scores:";
    for (PieceType p = 0; p < 12; p++) {
      std::cout << pieceToString(p) << ": " << pieceScoreEarlyGame[p] << "\n";
    }

    /*std::cout << "\nState stack: ";
    for (int i = 0; i < (int)stack.getIndex(); i++) {
      auto node = stack.peekNodeAt(i);
      std::cout << "(" << (int)node.data[LAST_MOVED_INDEX] << ",";
      std::cout << (int)node.data[LAST_CAPTURED_INDEX] << ") ";
      std::cout << (int)node.data[EN_PASSANT_INDEX] << ") ";
    }*/
    std::cout << "\n" << fen();
    std::cout << "\n";
    std::cout << "\n";
    /*
    _movegen.reset(occupancy(White), attackMap);
    while (_movegen.hasNext()) {
      int src;
      int dest;
      _movegen.next(attackMap, src, dest);
      std::cout << src << "->" << dest << "\n";
    }
    */
  }
}

void Board::generateSpecialMoves(std::vector<Move> &sbuffer) {
  // fill specialbuffer
  // pawn moves: en passant and double moves
  // castles
  Color color = turn();
  u64 enemies = occupancy(flipColor(color));
  u64 friendlies = occupancy(color);
  u64 occ = enemies | friendlies;

  PieceType pawn = color == White ? W_Pawn : B_Pawn;
  int sRow = color == White ? 1 : 6;
  int pRow = color == White ? 6 : 1;

  std::array<int, 64> arr;
  int count;
  bitscanAllInt(arr, bitboard[pawn], count);
  for (int i = 0; i < count; i++) {
    int srci = arr[i];
    int row = intToRow(srci);
    if (boardState[EN_PASSANT_INDEX] >= 0 &&
        u64FromIndex(boardState[EN_PASSANT_INDEX]) & attackMap[srci]) {
      sbuffer.push_back(
          Move(srci, boardState[EN_PASSANT_INDEX], MoveTypeCode::EnPassant));
    }
    u64 singleMove = PAWN_MOVE_CACHE[srci][color] & ~occ;
    if (singleMove) {
      if (row == sRow) {
        u64 doubleMove = PAWN_DOUBLE_CACHE[srci][color] & ~occ;
        if (doubleMove) {
          sbuffer.push_back(
              Move(srci, u64ToIndex(doubleMove), MoveTypeCode::DoublePawn));
        }
      }
      if (row == pRow) {
        int desti = u64ToIndex(singleMove);
        sbuffer.push_back(Move(srci, desti, MoveTypeCode::BPromotion));
        sbuffer.push_back(Move(srci, desti, MoveTypeCode::RPromotion));
        sbuffer.push_back(Move(srci, desti, MoveTypeCode::KPromotion));
        sbuffer.push_back(Move(srci, desti, MoveTypeCode::QPromotion));
      } else {
        sbuffer.push_back(
            Move(srci, u64ToIndex(singleMove), MoveTypeCode::Default));
      }
    }
  }

  if (!isCheck()) {
    PieceType myKing = color == White ? W_King : B_King;
    int myKingIndex = u64ToIndex(bitboard[myKing]);
    size_t myLongIndex = B_LONG_INDEX;
    size_t myShortIndex = B_SHORT_INDEX;
    if (myKing == W_King) {
      myLongIndex = W_LONG_INDEX;
      myShortIndex = W_SHORT_INDEX;
    }
    Color opponent = flipColor(color);
    if (boardState[myLongIndex]) {               // is allowed
      if (!(CASTLE_LONG_SQUARES[color] & occ)) { // in-between is empty
        if (!_isUnderAttack(CASTLE_LONG_KING_SLIDE[color], opponent)) {
          sbuffer.push_back(Move(myKingIndex,
                                 u64ToIndex(CASTLE_LONG_KING_DEST[color]),
                                 MoveTypeCode::CastleLong));
        }
      }
    }
    if (boardState[myShortIndex]) {               // is allowed
      if (!(CASTLE_SHORT_SQUARES[color] & occ)) { // in-between is empty
        if (!_isUnderAttack(CASTLE_SHORT_KING_SLIDE[color], opponent)) {
          sbuffer.push_back(Move(myKingIndex,
                                 u64ToIndex(CASTLE_SHORT_KING_DEST[color]),
                                 MoveTypeCode::CastleShort));
        }
      }
    }
  }
}

Move Board::nextMove(LazyMovegen &movegen, std::vector<Move> &sbuffer,
                     bool &hasGenSpecial) {
  if (movegen.hasNext()) {
    int s, d;
    movegen.next(attackMap, s, d);
    // std::cout << s << "->" << d <<"\n";
    u64 friendlies = occupancy(turn());
    if (u64FromIndex(d) & friendlies) {
      return nextMove(movegen, sbuffer, hasGenSpecial);
    } else {
      Move mv;
      Color color = turn();
      PieceType pawn = color == White ? W_Pawn : B_Pawn;
      int pRow = color == White ? 6 : 1;
      if (u64FromIndex(s) & bitboard[pawn]) {
        u64 enemies = occupancy(flipColor(color));
        // deal with pawn capture rules
        if (u64FromIndex(d) & enemies) {
          if (intToRow(s) == pRow) {
            sbuffer.push_back(Move(s, d, MoveTypeCode::BPromotion));
            sbuffer.push_back(Move(s, d, MoveTypeCode::RPromotion));
            sbuffer.push_back(Move(s, d, MoveTypeCode::KPromotion));
            sbuffer.push_back(Move(s, d, MoveTypeCode::QPromotion));
            return nextMove(movegen, sbuffer, hasGenSpecial);
          } else {
            mv = Move(s, d, MoveTypeCode::Default);
          }
        } else {
          return nextMove(movegen, sbuffer, hasGenSpecial);
        }
      } else {
        mv = Move(s, d, MoveTypeCode::Default);
      }
      if (verifyLegal(mv)) {
        return mv;
      } else {
        return nextMove(movegen, sbuffer, hasGenSpecial);
      }
    }
  } else {
    if (!hasGenSpecial) {
      generateSpecialMoves(sbuffer);
      hasGenSpecial = true;
    }
    while (sbuffer.size() > 0) {
      Move mv = sbuffer.back();
      sbuffer.pop_back();
      if (verifyLegal(mv)) {
        return mv;
      }
    }
    // no more
    return Move(); // null ends the iterator;
  }
}

Move Board::lastMove() {
  if (stack.canPop()) {
    return stack.peek().mv;
  } else {
    return Move();
  }
}

u64 Board::_knightAttacks(u64 index64) {
  u64 ray = 0;
  int index = u64ToIndex(index64);
  u64 result = 0;
  ray = KNIGHT_MOVE_CACHE[index];
  result |= ray;
  return result;
}

u64 Board::_pawnMoves(u64 index64, Color color, u64 occupants) {
  int index = u64ToIndex(index64);
  u64 result = 0;
  u64 forward1 = PAWN_MOVE_CACHE[index][color];
  if (forward1 & ~occupants) { // if one forward is empty
    result |= forward1;
    if ((color == White && u64ToRow(index64) == 1) ||
        (color == Black && u64ToRow(index64) == 6)) {
      u64 forward2 = PAWN_DOUBLE_CACHE[index][color];
      if (!((forward1 | forward2) & occupants)) { // two forward are empty
        result |= forward2;
      }
    }
  }
  return result;
}

u64 Board::_pawnAttacks(u64 index64, Color color, u64 enemies) {
  int index = u64ToIndex(index64);
  u64 result = 0;
  u64 captures = PAWN_CAPTURE_CACHE[index][color];
  result |= captures & enemies; // plus en passant sqaure
  if (color == turn() && boardState[EN_PASSANT_INDEX] >= 0) {
    u64 epSquare = u64FromIndex(boardState[EN_PASSANT_INDEX]);
    result |= captures & epSquare;
  }
  return result;
}

u64 Board::_kingAttacks(u64 index64) {
  u64 ray = 0;
  u64 result = 0;
  int index = u64ToIndex(index64);
  ray = KING_MOVE_CACHE[index];
  result |= ray;
  return result;
}

u64 Board::_rookRay(u64 origin, int direction, u64 mask) {
  u64 ray = ROOK_MOVE_CACHE[u64ToIndex(origin)][direction];
  u64 overlaps = ray & mask;
  if (overlaps) {
    if (direction < 2) {
      ray &= ~ROOK_MOVE_CACHE[bitscanForward(overlaps)][direction];
    } else {
      ray &= ~ROOK_MOVE_CACHE[bitscanReverse(overlaps)][direction];
    }
  }
  return ray;
}

u64 Board::_bishopRay(u64 origin, int direction, u64 mask) {
  u64 ray = BISHOP_MOVE_CACHE[u64ToIndex(origin)][direction];
  u64 overlaps = ray & mask;
  if (direction < 2) {
    if (overlaps) {
      ray &= ~BISHOP_MOVE_CACHE[bitscanForward(overlaps)][direction];
    }
  } else {
    if (overlaps) {
      ray &= ~BISHOP_MOVE_CACHE[bitscanReverse(overlaps)][direction];
    }
  }
  return ray;
}

u64 Board::_rookAttacks(u64 index64, u64 occupants) {
  // given a color rook at index64
  int index = u64ToIndex(index64);
  u64 ray = 0;
  u64 overlaps = 0;
  u64 result = 0;

  ray = ROOK_MOVE_CACHE[index][0];
  overlaps = ray & occupants;
  result |= ray;
  if (overlaps) {
    result &= ~ROOK_MOVE_CACHE[bitscanForward(overlaps)][0];
  }

  ray = ROOK_MOVE_CACHE[index][1];
  overlaps = ray & occupants;
  result |= ray;
  if (overlaps) {
    result &= ~ROOK_MOVE_CACHE[bitscanForward(overlaps)][1];
  }

  ray = ROOK_MOVE_CACHE[index][2];
  overlaps = ray & occupants;
  result |= ray;
  if (overlaps) {
    result &= ~ROOK_MOVE_CACHE[bitscanReverse(overlaps)][2];
  }

  ray = ROOK_MOVE_CACHE[index][3];
  overlaps = ray & occupants;
  result |= ray;
  if (overlaps) {
    result &= ~ROOK_MOVE_CACHE[bitscanReverse(overlaps)][3];
  }

  return result;
}

u64 Board::_bishopAttacks(u64 index64, u64 occupants) {
  // given a color bishop at index64
  int index = u64ToIndex(index64);
  u64 ray = 0;
  u64 result = 0;
  u64 overlaps = 0;

  ray = BISHOP_MOVE_CACHE[index][0];
  overlaps = ray & occupants;
  result |= ray;
  if (overlaps) {
    result &= ~BISHOP_MOVE_CACHE[bitscanForward(overlaps)][0];
  }

  ray = BISHOP_MOVE_CACHE[index][1];
  overlaps = ray & occupants;
  result |= ray;
  if (overlaps) {
    result &= ~BISHOP_MOVE_CACHE[bitscanForward(overlaps)][1];
  }

  ray = BISHOP_MOVE_CACHE[index][2];
  overlaps = ray & occupants;
  result |= ray;
  if (overlaps) {
    result &= ~BISHOP_MOVE_CACHE[bitscanReverse(overlaps)][2];
  }

  ray = BISHOP_MOVE_CACHE[index][3];
  overlaps = ray & occupants;
  result |= ray;
  if (overlaps) {
    result &= ~BISHOP_MOVE_CACHE[bitscanReverse(overlaps)][3];
  }
  return result;
}

u64 Board::occupancy(Color color) {
  // cache this?
  if (color == White) {
    return bitboard[W_King] | bitboard[W_Queen] | bitboard[W_Knight] |
           bitboard[W_Bishop] | bitboard[W_Rook] | bitboard[W_Pawn];
  } else {
    return bitboard[B_King] | bitboard[B_Queen] | bitboard[B_Knight] |
           bitboard[B_Bishop] | bitboard[B_Rook] | bitboard[B_Pawn];
  }
}

u64 Board::occupancy() { return occupancy(White) | occupancy(Black); }

PieceType
Board::_leastValuablePiece(u64 sqset, Color color,
                           u64 &outposition) { // returns the least valuable
                                               // piece of color color in sqset
  PieceType s = pieceIndexFromColor(color);
  for (PieceType p = s; p < s + 6; p++) {
    u64 mask = bitboard[p] & sqset;
    if (mask) {
      outposition = u64FromIndex(bitscanForward(mask)); // pop out any position
      return p;
    }
  }
  outposition = 0;
  return Empty;
}

int Board::see(Move mv) {
  u64 src = mv.getSrc();
  u64 dest = mv.getDest();
  PieceType attacker = pieceAt(src);
  PieceType targetPiece = pieceAt(dest);

  Color color = colorOf(attacker);
  u64 attackSet = _isUnderAttack(dest);
  u64 usedAttackers = src;
  u64 occ = occupancy() & ~src; //"move" from src

  int scores[32];
  int depth = 0;
  scores[0] = MATERIAL_TABLE[targetPiece]; // first player is up by capturing
  color = flipColor(color);
  // std::cout << "\nscores[" << depth << "] = " << scores[depth] << "\n";
  PieceType piece = attacker; // piece at dest

  std::array<u64, 64> arr;
  int count;

  do {
    depth++;
    scores[depth] = MATERIAL_TABLE[piece] - scores[depth - 1]; // capture!
    // std::cout << "\npiece = " << pieceToString(piece) << "\n";
    // std::cout << "scores[" << depth << "] = " << scores[depth] << "\n";

    if (max(-1 * scores[depth - 1], scores[depth]) < 0)
      break;

    // add x-ray or conditional attackers to attack set
    // add pawns
    bitscanAll(arr, bitboard[W_Pawn], count);
    for (int i = 0; i < count; i++) {
      u64 loc = arr[i];
      if ((loc & usedAttackers) == 0) {
        if (PAWN_CAPTURE_CACHE[u64ToIndex(loc)][White] & dest) {
          attackSet |= loc;
        }
      }
    }
    bitscanAll(arr, bitboard[B_Pawn], count);
    for (int i = 0; i < count; i++) {
      u64 loc = arr[i];
      if ((loc & usedAttackers) == 0) {
        if (PAWN_CAPTURE_CACHE[u64ToIndex(loc)][Black] & dest) {
          attackSet |= loc;
        }
      }
    }
    bitscanAll(arr, bitboard[W_Rook] | bitboard[B_Rook], count);
    for (int i = 0; i < count; i++) {
      u64 loc = arr[i];
      if ((loc & usedAttackers) == 0) {
        for (int d = 0; d < 4; d++) {
          if (_rookRay(loc, d, occ) & dest) {
            attackSet |= loc;
          }
        }
      }
    }
    bitscanAll(arr, bitboard[W_Bishop] | bitboard[B_Bishop], count);
    for (int i = 0; i < count; i++) {
      u64 loc = arr[i];
      if ((loc & usedAttackers) == 0) {
        for (int d = 0; d < 4; d++) {
          if (_bishopRay(loc, d, occ) & dest) {
            attackSet |= loc;
          }
        }
      }
    }
    bitscanAll(arr, bitboard[W_Queen] | bitboard[B_Queen], count);
    for (int i = 0; i < count; i++) {
      u64 loc = arr[i];
      if ((loc & usedAttackers) == 0) {
        for (int d = 0; d < 4; d++) {
          if (_bishopRay(loc, d, occ) & dest) {
            attackSet |= loc;
          }
          if (_rookRay(loc, d, occ) & dest) {
            attackSet |= loc;
          }
        }
      }
    }

    u64 attPos = 0;
    attackSet = attackSet & ~usedAttackers; // remove attacker from attack set

    piece = _leastValuablePiece(attackSet, color, attPos);
    usedAttackers |= attPos;
    occ &= ~attPos;
    color = flipColor(color);
  } while (piece != Empty);
  while (--depth) {
    scores[depth - 1] = -1 * max(-1 * scores[depth - 1], scores[depth]);
  }
  return scores[0];
  // static exch eval for move ordering and quiescience pruning
}

bool Board::verifyLegal(Move mv) {
  if (mv.getTypeCode() == MoveTypeCode::CastleLong ||
      mv.getTypeCode() == MoveTypeCode::CastleShort) {
    return true; // castling is verified on add
  }
  Color c = turn();
  return !isCheckingMove(mv, flipColor(c), c);
}

bool Board::isCheckingMove(Move mv) {
  return isCheckingMove(mv, turn(), flipColor(turn()));
}

std::string Board::vectorize() {
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

bool Board::isCheckingMove(Move mv, Color aColor, Color kingColor) {
  // is King Color's king under attack after move is made by myColor
  bool result = false;
  PieceType king = kingColor == White ? W_King : B_King;

  u64 src = mv.getSrc();
  u64 dest = mv.getDest();
  u64 destFormer = pieceAt(dest);
  PieceType mover = pieceAt(src);
  int moveType = mv.getTypeCode();

  Color moveColor = turn(); // in theory this could be diff

  bitboard[mover] &= ~src;

  if (moveType == MoveTypeCode::EnPassant) {
    if (mover == W_Pawn) {
      u64 capturedPawn = PAWN_MOVE_CACHE[u64ToIndex(dest)][Black];
      bitboard[B_Pawn] &= ~capturedPawn;
    }
    if (mover == B_Pawn) {
      u64 capturedPawn = PAWN_MOVE_CACHE[u64ToIndex(dest)][White];
      bitboard[W_Pawn] &= ~capturedPawn;
    }
  } else {
    if (destFormer != Empty) {
      bitboard[destFormer] &= ~dest;
    }
  }

  // move to new location
  if (mv.isPromotion()) {
    bitboard[mv.getPromotingPiece(moveColor)] |= dest;
  } else {
    bitboard[mover] |= dest;
  }

  if (moveType == MoveTypeCode::CastleLong) {
    if (mover == W_King) {
      bitboard[W_Rook] &= ~rookStartingPositions[White][0];
      bitboard[W_Rook] |= CASTLE_LONG_ROOK_DEST[White];
    } else {
      bitboard[B_Rook] &= ~rookStartingPositions[Black][0];
      bitboard[B_Rook] |= CASTLE_LONG_ROOK_DEST[Black];
    }
  } else if (moveType == MoveTypeCode::CastleShort) {
    if (mover == W_King) {
      bitboard[W_Rook] &= ~rookStartingPositions[White][1];
      bitboard[W_Rook] |= CASTLE_SHORT_ROOK_DEST[White];
    } else {
      bitboard[B_Rook] &= ~rookStartingPositions[Black][1];
      bitboard[B_Rook] |= CASTLE_SHORT_ROOK_DEST[Black];
    }
  }

  // 2. Check if king is in line of fire

  // ray out, knight out, king out, pawn out
  PieceType offset = 0;
  if (aColor == Black) {
    offset = 6;
  }
  PieceType eknight = W_Knight + offset;
  PieceType eking = W_King + offset;
  PieceType erook = W_Rook + offset;
  PieceType ebishop = W_Bishop + offset;
  PieceType equeen = W_Queen + offset;
  PieceType epawn = W_Pawn + offset;

  u64 kingBB = bitboard[king];
  int kingIndex = u64ToIndex(kingBB);
  u64 occ = occupancy();

  if (PAWN_CAPTURE_CACHE[kingIndex][kingColor] & bitboard[epawn] ||
      (KNIGHT_MOVE_CACHE[kingIndex] & bitboard[eknight] ||
       KING_MOVE_CACHE[kingIndex] & bitboard[eking])) {
    result = true;
  }

  for (int d = 0; d < 4; d++) {
    u64 ray;
    ray = _bishopRay(kingBB, d, occ);
    if ((bitboard[ebishop] | bitboard[equeen]) & ray) {
      result = true;
      break;
    }
    ray = _rookRay(kingBB, d, occ);
    if ((bitboard[erook] | bitboard[equeen]) & ray) {
      result = true;
      break;
    }
  }

  // 3. Revert bitboards

  if (moveType == MoveTypeCode::CastleLong ||
      moveType == MoveTypeCode::CastleShort) {
    if (mover == W_King) {
      if (moveType == MoveTypeCode::CastleLong) {
        bitboard[W_Rook] &= ~CASTLE_LONG_ROOK_DEST[White];
        bitboard[W_Rook] |= rookStartingPositions[White][0];
      } else {
        bitboard[W_Rook] &= ~CASTLE_SHORT_ROOK_DEST[White];
        bitboard[W_Rook] |= rookStartingPositions[White][1];
      }
    } else {
      if (moveType == MoveTypeCode::CastleLong) {
        bitboard[B_Rook] &= ~CASTLE_LONG_ROOK_DEST[Black];
        bitboard[B_Rook] |= rookStartingPositions[Black][0];
      } else {
        bitboard[B_Rook] &= ~CASTLE_SHORT_ROOK_DEST[Black];
        bitboard[B_Rook] |= rookStartingPositions[Black][1];
      }
    }
  }

  // move piece to old src
  bitboard[mover] |= src;

  // mask out dest
  if (mv.isPromotion()) {
    bitboard[mv.getPromotingPiece(moveColor)] &= ~dest;
  } else {
    bitboard[mover] &= ~dest;
  }

  // restore dest
  if (moveType == MoveTypeCode::EnPassant) { // instead of restoring at capture
                                             // location, restore one above
    if (mover == W_Pawn) {
      bitboard[B_Pawn] |= PAWN_MOVE_CACHE[u64ToIndex(dest)][Black];
    } else {
      bitboard[W_Pawn] |= PAWN_MOVE_CACHE[u64ToIndex(dest)][White];
    }
  } else if (destFormer != Empty) {
    bitboard[destFormer] |= dest;
  }
  return result;
}

std::vector<Move> Board::legalMoves() {
  std::vector<Move> v;
  LazyMovegen movegen(occupancy(turn()), attackMap);
  std::vector<Move> sbuffer;
  bool hasGenSpecial = false;
  Move mv = nextMove(movegen, sbuffer, hasGenSpecial);
  while (mv.notNull()) {
    v.push_back(mv);
    mv = nextMove(movegen, sbuffer, hasGenSpecial);
  }
  return v;
}

Color Board::turn() { return boardState[TURN_INDEX]; }

void Board::loadPosition(std::string fen) {
  PieceType piecelist[64];
  auto elems = tokenize(fen);

  int i = 0;
  int row = 7;
  int col = 0;
  while (row > 0 || col < 8) {
    std::string ch(1, elems[0][i]);
    if (ch == "/") {
      row -= 1;
      col = 0;
    } else {
      PieceType p = pieceFromString(ch);
      if (p == Empty) {
        int count = std::stoi(ch);
        for (int k = 0; k < count; k++) {
          piecelist[row * 8 + col] = p;
          col += 1;
        }
      } else {
        piecelist[row * 8 + col] = p;
        col += 1;
      }
    }
    i += 1;
  }

  Color t;
  if (elems[1] == "w") {
    t = White;
  } else {
    t = Black;
  }

  int wlong, wshort, blong, bshort;
  wlong = 0;
  wshort = 0;
  blong = 0;
  bshort = 0;
  for (int k = 0; k < (int)elems[2].size(); k++) {
    char ch = elems[2][k];
    if (ch == 'K') {
      wshort = 1;
    } else if (ch == 'Q') {
      wlong = 1;
    } else if (ch == 'k') {
      bshort = 1;
    } else if (ch == 'q') {
      blong = 1;
    }
  }

  int epIndex = -1;
  if (elems[3] != "-") {
    epIndex = indexFromSquareName(elems[3]);
  }

  loadPosition(piecelist, t, epIndex, wlong, wshort, blong, bshort);
}

void Board::loadPosition(PieceType *piecelist, Color turn, int epIndex,
                         int wlong, int wshort, int blong, int bshort) {
  // set board, bitboards
  for (PieceType i = 0; i < 12; i++) {
    bitboard[i] = 0;
    pieceScoreEarlyGame[i] = 0; 
    pieceScoreLateGame[i] = 0; 
  }
  _zobristHash = 0; // ZERO OUT

  for (int i = 0; i < 64; i++) {
    PieceType piece = piecelist[i];

    if (piece != Empty) {
      _addPiece(piece, u64FromIndex(i));
    }
  }
  boardState[TURN_INDEX] = White;
  boardState[EN_PASSANT_INDEX] = -1;
  boardState[W_LONG_INDEX] = 1;
  boardState[W_SHORT_INDEX] = 1;
  boardState[B_LONG_INDEX] = 1;
  boardState[B_SHORT_INDEX] = 1;
  boardState[LAST_CAPTURED_INDEX] = Empty;
  boardState[LAST_MOVED_INDEX] = Empty;
  boardState[HAS_REPEATED_INDEX] = 0;

  _switchTurn(turn);
  _setEpSquare(epIndex);
  _setCastlingPrivileges(White, wlong, wshort);
  _setCastlingPrivileges(Black, blong, bshort);

  stack.clear();
  _pseudoStack.clear();

  _generatePseudoLegal();

  _status = BoardStatus::NotCalculated;
}

void Board::reset() {
  rookStartingPositions[White][0] = u64FromIndex(0);
  rookStartingPositions[White][1] = u64FromIndex(7);
  rookStartingPositions[Black][0] = u64FromIndex(56);
  rookStartingPositions[Black][1] = u64FromIndex(63);
  kingStartingPositions[White] = u64FromIndex(4);
  kingStartingPositions[Black] = u64FromIndex(60);
  loadPosition(CLASSICAL_BOARD, White, -1, 1, 1, 1, 1);
}
