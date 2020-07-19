#ifndef AI_HPP
#define AI_HPP
#include <atomic>
#include <chrono>
#include <game/board.hpp>
#include <limits>
#include <thread>
#include <unordered_map>

const size_t TABLE_SIZE = 32768;
const size_t CAPACITY = 32768;

const size_t MINI_TABLE_SIZE = 8192;

enum NodeType { PV = 0, Cut = 1, All = 2 };

struct TableNode {
  u64 hash;
  int depth;

  Move bestMove;
  NodeType nodeType;

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

void sendPV(Board &board, int depth, Move &pvMove, int nodeCount, int score,
            std::chrono::_V2::system_clock::time_point start);

class TranspositionTable {
  TableBucket _arr[TABLE_SIZE];
  size_t members;

public:
  TranspositionTable() { members = 0; }

  int ppm() { return ((double)members / (double)CAPACITY) * 1000.0; }

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

class MiniTable {
  TableBucket _arr[MINI_TABLE_SIZE];

public:
  MiniTable() {}

  TableBucket *find(TableNode &node) {
    u64 hashval = node.hash;
    int bucketIndex = hashval % MINI_TABLE_SIZE;
    TableBucket *bucket = _arr + bucketIndex;
    if (bucket->first == node) {
      return bucket;
    }
    return NULL;
  }

  TableBucket *end() { return NULL; }

  void clear() {
    for (size_t i = 0; i < MINI_TABLE_SIZE; i++) {
      _arr[i].first.hash = 0;
    }
  }

  void insert(TableNode &node, int score) {
    u64 hashval = node.hash;
    int bucketIndex = hashval % MINI_TABLE_SIZE;
    TableBucket *bucket = _arr + bucketIndex;
    bucket->first = node;
    bucket->second = score;
  }
};

namespace AI {
int materialEvaluation(Board &board);
int evaluation(Board &board);
int flippedEval(Board &board);

TranspositionTable &getTable();

Move rootMove(Board &board, int depth, std::atomic<bool> &stop, int &outscore,
              Move &prevPv, int &count,
              std::chrono::_V2::system_clock::time_point start);

int quiescence(Board &board, int plyCount, int alpha, int beta,
               std::atomic<bool> &stop, int &count, int depthLimit, int kickoff);

int alphaBetaNega(Board &board, int depth, int plyCount, int alpha, int beta,
                  std::atomic<bool> &stop, int &count, NodeType myNodeType);

void orderMoves(Board &board, std::vector<Move> &mvs, int ply);

void init();
void reset();

void clearKillerTable();

int mobility(Board &board, Color c);

float kingSafety(Board &board, Color c);

} // namespace AI

#endif
