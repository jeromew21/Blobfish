#ifndef AI_HPP
#define AI_HPP
#include <atomic>
#include <thread>
#include <chrono>
#include <limits>
#include <game/board.hpp>
#include <unordered_map>

const size_t TABLE_SIZE = 16384;
const size_t CAPACITY = 16384;

enum NodeType {
    PV = 0, Cut = 1, All = 2
};

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

    bool operator==(const TableNode &other) const {
        return (hash == other.hash);
    }

    TableNode() {
        hash = 0;
    }
};

struct TableBucket {
    TableNode first;
    int second;

    TableBucket() {}

};

void sendPV(Board &board, int depth, int nodeCount, int score, int time);

class TranspositionTable {
    TableBucket _arr[TABLE_SIZE];
    size_t members;

    public: 
        TranspositionTable() {
            members = 0;
        }

        int ppm() {
            return ((double) members / (double) CAPACITY) * 1000.0;
        }


        TableBucket* find(TableNode &node) {
            u64 hashval = node.hash;
            int bucketIndex = hashval % TABLE_SIZE;
            TableBucket* bucket = _arr + bucketIndex;
            if (bucket->first == node) {
                return bucket;
            }
            return NULL;
        }

        TableBucket* end() {
            return NULL;
        }

        void insert(TableNode &node, int score) {
            u64 hashval = node.hash;
            int bucketIndex = hashval % TABLE_SIZE;
            TableBucket* bucket = _arr + bucketIndex;
            if (bucket->first.hash == 0) {
                //no overwrite
                members += 1;
            }
            if (members > CAPACITY) {
                //evict
            }
            bucket->first = node;
            bucket->second = score;
        }
};

namespace AI {
    int materialEvaluation(Board &board);
    int evaluation(Board &board);
    int flippedEval(Board &board);

    TranspositionTable& getTable();

    Move rootMove(Board &board, int depth, std::atomic<bool> &stop, int &outscore);

    int quiescence(Board &board, int plyCount, int alpha, int beta, std::atomic<bool> &stop, int &count);

    int alphaBetaNega(Board &board, int depth, int alpha, int beta, std::atomic<bool> &stop, int &count, bool isPv);

    void orderMoves(Board &board, std::vector<Move> &mvs);

    void init();
}



#endif
