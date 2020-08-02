#ifndef AI_HPP
#define AI_HPP
#include <atomic>
#include <chrono>
#include <game/board.hpp>
#include <limits>
#include <thread>

const size_t TABLE_SIZE = 1048576;

const size_t MINI_TABLE_SIZE = 16384;

struct TableNode {
  u64 hash;
  int depth;
  NodeType nodeType;
  Move bestMove;

  TableNode(Board &board, int d, NodeType typ) {
    hash = board.zobrist();
    depth = d;
    nodeType = typ;
    bestMove = Move::NullMove();
  }

  bool operator==(const TableNode &other) const { return (hash == other.hash); }

  TableNode() { hash = 0; }
};

struct TableBucket {
  TableNode first;
  int second;

  TableBucket() {}
};

class TranspositionTable {
  TableBucket _arr[TABLE_SIZE];
  size_t members;

public:
  TranspositionTable() { members = 0; }

  int ppm() { return ((double)members / (double)TABLE_SIZE) * 1000.0; }

  TableBucket *find(TableNode &node) {
    u64 hashval = node.hash;
    int bucketIndex = hashval % TABLE_SIZE;
    TableBucket *bucket = _arr + bucketIndex;
    if (bucket->first == node) {
      return bucket;
    }
    return NULL;
  }

  TableBucket *end() { return NULL; }

  void insert(TableNode &node, int score) {
    u64 hashval = node.hash;
    int bucketIndex = hashval % TABLE_SIZE;
    TableBucket *bucket = _arr + bucketIndex;
    if (bucket->first.hash == 0) {
      // no overwrite
      members += 1;
    }
    if (node.hash == bucket->first.hash) {
      if (node.depth < bucket->first.depth) {
        return;
      }
    }
    bucket->first = node;
    bucket->second = score;
  }
};

struct MiniTableBucket {
  u64 hash; //position hash
  int depth; //number of plies to root saved
  std::array<Move, 64> seq;
};

class MiniTable {
  MiniTableBucket _arr[MINI_TABLE_SIZE];

public:
  MiniTable() {}

  MiniTableBucket *find(u64 hashval) {
    int bucketIndex = hashval % MINI_TABLE_SIZE;
    MiniTableBucket *bucket = _arr + bucketIndex;
    if (hashval == bucket->hash) {
      return bucket;
    }
    return NULL;
  }

  MiniTableBucket *end() { return NULL; }

  void clear() {
    for (size_t i = 0; i < MINI_TABLE_SIZE; i++) {
      _arr[i].hash = 0;
    }
  }

  void insert(u64 hashval, int depth, std::array<Move, 64> *moveseq) {
    int bucketIndex = hashval % MINI_TABLE_SIZE;
    MiniTableBucket *bucket = _arr + bucketIndex;
    bucket->hash = hashval;
    bucket->seq = *moveseq;
    bucket->depth = depth;
  }
};

namespace AI {
int materialEvaluation(Board &board);
int evaluation(Board &board);
int flippedEval(Board &board);

TranspositionTable &getTable();


void sendPV(Board &board, int depth, Move pvMove, int nodeCount, int score,
            std::chrono::_V2::system_clock::time_point start);

Move rootMove(Board &board, int depth, std::atomic<bool> &stop, int &outscore,
              Move prevPv, int &count,
              std::chrono::_V2::system_clock::time_point start, std::vector<MoveScore>& prevScores);

int quiescence(Board &board, int depth, int plyCount, int alpha, int beta,
               std::atomic<bool> &stop, int &count, int kickoff);

int alphaBetaNega(Board &board, int depth, int plyCount, int alpha, int beta,
                  std::atomic<bool> &stop, int &count, NodeType myNodeType);

void init();
void reset();

void clearKillerTable();

int mobility(Board &board, Color c);

} // namespace AI

#endif
