#ifndef AI_HPP
#define AI_HPP
#include <atomic>
#include <thread>
#include <chrono>
#include <limits>
#include <game/board.hpp>
#include <unordered_map>

const size_t TABLE_SIZE = 16384;

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
};

void sendPV(Board &board, int depth, int nodeCount, int score, int time);

namespace std {
  template <> struct hash<TableNode> {
    std::size_t operator()(const TableNode& k) const {
      return k.hash;
    }
  };
}

class TranspositionTable {
    std::unordered_map<TableNode, int> map;
    public: 
        int get(TableNode &node) {
            return map.at(node);
        }

        std::unordered_map<TableNode, int>::iterator find(TableNode &node) {
            return map.find(node);
        }

        std::unordered_map<TableNode, int>::iterator end() {
            return map.end();
        }

        bool contains(TableNode &node) {
            return map.find(node) != map.end();
        }

        void insert(TableNode &node, int score) {
            map.erase(node);
            map.insert({node, score});
        }

        void erase(TableNode &node) {
            map.erase(node);
        }
};

namespace AI {
    int max(int i1, int i2);
    int min(int i1, int i2);
    
    int materialEvaluation(Board &board);
    int evaluation(Board &board);
    int flippedEval(Board &board);

    TranspositionTable& getTable();

    Move rootMove(Board &board, int depth, std::atomic<bool> &stop, int &outscore);

    Move randomMove(Board &board);

    int quiescence(Board &board, int plyCount, int alpha, int beta, std::atomic<bool> &stop, int &count);

    int alphaBetaNega(Board &board, int depth, int alpha, int beta, std::atomic<bool> &stop, int &count, bool isPv);

    void init();
}



#endif
