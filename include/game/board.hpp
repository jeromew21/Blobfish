#ifndef BOARD_HPP
#define BOARD_HPP

#include <iostream>
#include <unordered_set>
#include <game/backend.hpp>


void populateMoveCache();
void initializeZobrist();

class Board
{
  private:
    std::vector<PieceIndexTuple> _pieceLocations; //update on resetMoveIterator
    void _findPieceLocations();
    std::vector<Move> _moveset; //for lazy generator
    size_t _pieceIndex; //which piece are we on?
    size_t _movesetIndex; //which index within the current moveset are we on?
    bool _nextMoveset(); //grab the next one
    bool _pieceUnderAttack(Color color, u64 pieceBitboard);
    bool _legalMovesDirty;
    std::vector<Move> _legalMovesCache;

    std::unordered_set<u64> _threefoldMap;
    bool _threefoldFlag;


    //incremental update zobrist methods
    void _removePiece(PieceType p, u64 location);
    void _addPiece(PieceType p, u64 location);
    void _setCastlingPrivileges(Color color, int cLong, int cShort);
    void _setEpSquare(int sq);
    void _switchTurn();
    void _switchTurn(Color t);

    u64 _zobristHash;

  public:
    BoardStateStack stack;
    u64 bitboard[12];

    u64 rookStartingPositions[2][2];
    u64 kingStartingPositions[2];

    int boardState[BOARD_STATE_ENTROPY];
    u64 occupancy();
    u64 occupancy(Color color); //COSTLY????

    void resetMoveIterator();
    Move nextMove(); //lazy generator


    //Important stuff
    std::vector<Move> legalMoves();
    Color turn();
    u64 zobrist();

    bool isTerminal();


    //costly calls
    bool isInCheck(Color color);
    PieceType pieceAt(u64 space);
    int material(Color color);
    int material();
    std::string fen();
    BoardStatus status();
  
    //STATE CHANGERS
    void _resetLegalMovesCache();
    void reset();
    void makeMove(Move mv);
    void makeMove(Move mv, bool dirty);
    void unmakeMove();
    void unmakeMove(bool dirty);
    void loadPosition(PieceType* piecelist, Color turn, int epIndex, int wlong, int wshort, int blong, int bshort);
    void loadPosition(std::string fen);

    //interface methods
    void dump(bool debug);
    void dump();
    bool canUndo();
    Move moveFromAlgebraic(const std::string& alg);
    std::string moveToExtAlgebraic(const Move &mv);
    std::string moveToUCIAlgebraic(const Move &mv);
    std::string moveToAlgebraic(const Move &mv);

    Board() {
      reset();
      debugLog("Created a board.");
    }
};

#endif
