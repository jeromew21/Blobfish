#ifndef BOARD_HPP
#define BOARD_HPP

#include <game/backend.hpp>
#include <iostream>
#include <unordered_set>

void populateMoveCache();
void initializeZobrist();
std::string moveToUCIAlgebraic(Move mv);

class Board {
private:
  bool _statusDirty;
  BoardStatus _status;

  std::vector<PseudoLegalData> _pseudoStack;

  // incremental update zobrist methods
  void _removePiece(PieceType p, u64 location);
  void _addPiece(PieceType p, u64 location);
  void _setCastlingPrivileges(Color color, int cLong, int cShort);
  void _setEpSquare(int sq);
  void _switchTurn();
  void _switchTurn(Color t);

  u64 _zobristHash;

  u64 _isUnderAttack(u64 target); // return a set that attack the target
  u64 _isUnderAttack(
      u64 target, Color byWho); // return a set of pieces that attack the target
  PieceType
  _leastValuablePiece(u64 sqset, Color color,
                      u64 &outposition); // returns the least valuable piece of
                                         // color color in sqset

  void _generatePseudoLegal();

  u64 _bishopAttacks(u64 index64, u64 occupants);
  u64 _rookAttacks(u64 index64, u64 occupants);
  u64 _knightAttacks(u64 index64);
  u64 _kingAttacks(u64 index64);
  u64 _pawnAttacks(u64 index64, Color color, u64 enemies);
  u64 _pawnMoves(u64 index64, Color color, u64 occupants);

  u64 _rookRay(u64 origin, int direction, u64 mask);
  u64 _bishopRay(u64 origin, int direction, u64 mask);

public:
  BoardStateStack stack;
  u64 bitboard[12];

  std::array<u64, 64> attackMap;
  std::array<u64, 64> defendMap;

  bool verifyLegal(Move mv);

  void generateSpecialMoves(std::vector<Move> &sbuffer);
  Move nextMove(LazyMovegen &movegen, std::vector<Move> &sbuffer,
                bool &hasGenSpecial);

  u64 rookStartingPositions[2][2];
  u64 kingStartingPositions[2];

  int boardState[BOARD_STATE_ENTROPY];
  u64 occupancy();
  u64 occupancy(Color color); // COSTLY????

  int see(Move mv);

  std::string vectorize(); // stdout a string rep

  int mobility(Color color);

  // shortcut move gen
  std::vector<Move> produceUncheckMoves();

  bool isCheckingMove(Move mv, Color aColor, Color kingColor);
  bool isCheckingMove(Move mv);

  // Important stuff
  std::vector<Move> legalMoves(); // calls generate
  Color turn();
  u64 zobrist();

  bool isCheck();

  // costly calls
  PieceType pieceAt(u64 space);
  PieceType pieceAt(u64 space, Color color);
  int material(Color color);
  int material();
  std::string fen();
  BoardStatus status();

  Move lastMove();

  // STATE CHANGERS
  void reset();
  void makeMove(Move mv);
  void unmakeMove();
  void loadPosition(PieceType *piecelist, Color turn, int epIndex, int wlong,
                    int wshort, int blong, int bshort);
  void loadPosition(std::string fen);

  // interface methods
  void dump(bool debug);
  void dump();
  bool canUndo();
  Move moveFromAlgebraic(const std::string &alg);
  std::string moveToExtAlgebraic(Move mv);
  std::string moveToAlgebraic(Move mv);

  Board();
};

#endif
