#ifndef AI_HPP
#define AI_HPP
#include <atomic>
#include <chrono>
#include <game/board.hpp>
#include <limits>
#include <thread>

struct TableNode {
  u64 hash;
  int8_t depth;
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
  Score second;

  TableBucket() {}
};

template <int N> class TranspositionTable {
  TableBucket _arr[N];
  size_t members;

public:
  TranspositionTable() { members = 0; }

  int ppm() { return ((double)members / (double)N) * 1000.0; }

  TableBucket *find(TableNode &node) {
    u64 hashval = node.hash;
    int bucketIndex = hashval % N;
    TableBucket *bucket = _arr + bucketIndex;
    if (bucket->first == node) {
      return bucket;
    }
    return NULL;
  }

  TableBucket *end() { return NULL; }

  void insert(TableNode &node, Score score) {
    u64 hashval = node.hash;
    int bucketIndex = hashval % N;
    TableBucket *bucket = _arr + bucketIndex;
    if (bucket->first.hash == 0) {
      // no overwrite
      members += 1;
    }
    if (node.hash == bucket->first.hash) {    // same position
      if (node.depth < bucket->first.depth) { // already searched to higher
                                              // depth
        return;
      }
    }
    bucket->first = node;
    bucket->second = score;
  }
};

struct MiniTableBucket {
  u64 hash;  // position hash
  int depth; // number of plies to root saved
  std::array<Move, 64> seq;
};

template <int N> class MiniTable {
  MiniTableBucket _arr[N];
  int members;
public:
  MiniTable() {
    members = 0;
  }

  MiniTableBucket *find(u64 hashval) {
    int bucketIndex = hashval % N;
    MiniTableBucket *bucket = _arr + bucketIndex;
    if (hashval == bucket->hash) {
      return bucket;
    }
    return NULL;
  }

  int ppm() { return ((double)members / (double)N) * 1000.0; }

  MiniTableBucket *end() { return NULL; }

  void clear() {
    for (size_t i = 0; i < N; i++) {
      _arr[i].hash = 0;
    }
  }

  void insert(u64 hashval, int depth, std::array<Move, 64> *moveseq) {
    int bucketIndex = hashval % N;
    MiniTableBucket *bucket = _arr + bucketIndex;
    if (bucket->hash == 0) {
      // no overwrite
      members += 1;
    }
    bucket->hash = hashval;
    bucket->seq = *moveseq;
    bucket->depth = depth;
  }
};

namespace AI {
int materialEvaluation(Board &board);
int evaluation(Board &board);
int flippedEval(Board &board);

void sendPV(Board &board, int depth, Move pvMove, int nodeCount, Score score,
            std::chrono::_V2::system_clock::time_point start);

Move rootMove(Board &board, int depth, std::atomic<bool> &stop, Score &outscore,
              Move prevPv, int &count,
              std::chrono::_V2::system_clock::time_point start,
              std::vector<MoveScore> &prevScores);

Score quiescence(Board &board, int depth, int plyCount, Score alpha, Score beta,
               std::atomic<bool> &stop, int &count, int kickoff);

Score alphaBetaSearch(Board &board, int depth, int plyCount, Score alpha, Score beta,
                    std::atomic<bool> &stop, int &count, NodeType myNodeType,
                    bool isSave);

Score zeroWindowSearch(Board &board, int depth, int plyCount, Score beta,
                     std::atomic<bool> &stop, int &count, NodeType myNodeType);

std::vector<Move> generateMovesOrdered(Board &board, Move refMove, int plyCount,
                                       int &numPositiveMoves);

bool isCheckmateScore(Score sc);

void init();
void reset();

} // namespace AI

#endif
