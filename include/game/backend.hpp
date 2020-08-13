#ifndef DS_HPP
#define DS_HPP

#include <array>
#include <string.h>
#include <vector>

#include <game/move.hpp>

struct MoveScore {
  Move mv;
  int score;
  MoveScore(Move mv0, int score0) : mv(mv0), score(score0) {}
};

struct PieceSquareTable {
  std::array<float, 64> arr;

  float at(int index) { return arr[index]; }

  void dump() {
    for (int row = 7; row >= 0; row--) {
      for (int col = 0; col < 8; col++) {
        std::cout << (int)(10.0 * arr[intFromPair(row, col)]) << "|";
      }
      std::cout << "\n";
    }
    std::cout << "\n";
  }

  void set(int index, float score) { arr[index] = score; }
};

struct SpecialMoveBuffer {
  std::array<Move, 32> data; // Might overflow if given a crazy position.
  // Haven't done the math but shouldn't happen in any legal position.
  int size_;

  SpecialMoveBuffer() { size_ = 0; }

  Move back() {
    if (size_ > 0) {
      return data[size_ - 1];
    } else {
      throw;
    }
  }

  Move pop_back() {
    Move mv = back();
    size_ -= 1;
    return mv;
  }

  int size() { return size_; }

  void push_back(Move mv) {
    data[size_] = mv;
    size_ += 1;
  }
};

struct LazyMovegen {
  bool hasGenSpecial;
  SpecialMoveBuffer sbuffer;

  std::array<int, 64> srcList;
  int srcIndex;
  int numSrcs;

  std::array<int, 64> currDests;
  int currDestIndex;
  int numCurrDests;
  bool _hasNext;

  bool hasNext() { return _hasNext; }

  LazyMovegen(u64 srcMap, std::array<u64, 64> &attackMap) {
    hasGenSpecial = false;

    _hasNext = true;
    bitscanAllInt(srcList, srcMap, numSrcs);
    if (numSrcs == 0) {
      _hasNext = false;
    }
    srcIndex = 0;
    while (true) {
      if (srcIndex == numSrcs) {
        _hasNext = false;
        break;
      }
      bitscanAllInt(currDests, attackMap[srcList[srcIndex]], numCurrDests);
      currDestIndex = 0;
      if (numCurrDests == 0) {
        srcIndex += 1;
      } else {
        break;
      }
    }
  }

  void next(std::array<u64, 64> &attackMap, int &srcout, int &destout) {
    if (!hasNext()) {
      throw;
    }
    srcout = srcList[srcIndex];
    destout = currDests[currDestIndex];
    currDestIndex += 1;
    if (currDestIndex == numCurrDests) {
      srcIndex += 1;
      if (srcIndex == numSrcs) {
        _hasNext = false;
      } else {
        // load new
        while (true) {
          if (srcIndex == numSrcs) {
            _hasNext = false;
            break;
          }
          bitscanAllInt(currDests, attackMap[srcList[srcIndex]], numCurrDests);
          currDestIndex = 0;
          if (numCurrDests == 0) {
            srcIndex += 1;
          } else {
            break;
          }
        }
      }
    }
  }
};

struct BoardStateNode {
  int data[BOARD_STATE_ENTROPY];
  u64 hash;
  Move mv;
};

struct CounterMoveTable {
  std::array<std::array<Move, 64>, 64> arr[2];

  void clear() {
    for (int i = 0; i < 64; i++) {
      for (int k = 0; k < 64; k++) {
        arr[White][i][k] = Move();
      }
    }
  }

  bool contains(Move prev, Move mv, Color side) {
    return arr[side][prev.getSrcIndex()][prev.getDestIndex()] == mv;
  }

  void insert(Color side, Move prev, Move counter) {
    arr[side][prev.getSrcIndex()][prev.getDestIndex()] = counter;
  }
};

struct HistoryTable {
  std::array<std::array<int, 64>, 64> arr[2]; // to-from cutoff count

  void clear() {
    for (int i = 0; i < 64; i++) {
      for (int k = 0; k < 64; k++) {
        arr[White][i][k] = 0;
        arr[Black][i][k] = 0;
      }
    }
  }

  int get(Move mv, Color side) {
    return arr[side][mv.getSrcIndex()][mv.getDestIndex()];
  }

  void insert(Move mv, Color side, int depth) {
    arr[side][mv.getSrcIndex()][mv.getDestIndex()] += depth * depth;
  }
};

struct KillerTable {
  std::array<std::array<Move, 2>, 32> arr;

  void clear() {
    for (int k = 0; k < 32; k++) {
      arr[k][0] = Move::NullMove();
      arr[k][1] = Move::NullMove();
    }
  }

  bool contains(Move mv, int ply) {
    return arr[ply][0] == mv || arr[ply][1] == mv;
  }

  void insert(Move mv, int ply) {
    for (int i = 0; i < 2; i++) {
      if (arr[ply][i].isNull()) {
        arr[ply][i] = mv;
        return;
      } else if (arr[ply][i] == mv) {
        return;
      }
    }
    // otherwise replace
    arr[ply][0] = arr[ply][1];
    arr[ply][1] = mv;
  }
};

struct PseudoLegalData {
  std::array<u64, 64> aMap;
  std::array<u64, 64> dMap;

  PseudoLegalData(std::array<u64, 64> a, std::array<u64, 64> d) {
    aMap = a;
    dMap = d;
  }
};

class BoardStateStack {
private:
  size_t _index;
  std::vector<BoardStateNode> _data;

public:
  BoardStateStack() { _index = 0; }

  Move peekAt(int index) { return _data[index].mv; }

  BoardStateNode &peekNodeAt(int index) { return _data[index]; }

  void clear() {
    _index = 0;
    _data.clear();
  }

  BoardStateNode &peek() {
    if (_index == 0) {
      debugLog("pop from empty move stack");
      throw;
    }
    return _data.back();
  }

  void push(int *data, Move mv, u64 hash) {
    BoardStateNode node;
    for (size_t i = 0; i < BOARD_STATE_ENTROPY; i++) {
      node.data[i] = data[i];
    }
    node.mv = mv;
    node.hash = hash;
    _data.push_back(node);
    _index++;
  };

  bool canPop() { return _index > 0; }

  void pop() {
    if (_index == 0) {
      debugLog("pop from empty state stack");
      throw;
    }
    _data.pop_back();
    _index--;
  }

  size_t getIndex() { return _index; };
};

struct PerftCounter {
  u64 nodes;
  u64 captures;
  u64 ep;
  u64 castles;
  u64 promotions;
  u64 checks;
  u64 checkmates;
  PerftCounter() {
    nodes = 0;
    captures = 0;
    ep = 0;
    castles = 0;
    promotions = 0;
    checks = 0;
    checkmates = 0;
  }
};

template <int N> struct MoveVector {
  Move data[N];
  int size_;
  MoveVector() { size_ = 0; }

  void push_back(Move mv) {
    data[size_] = mv;
    size_++;
  }

  void pop_back() {
    if (size_ > 0) {
      size_--;
    } else {
      throw;
    }
  }

  Move back() {
    if (size_ > 0) {
      return data[size_ - 1];
    } else {
      throw;
    }
  }

  void clear() { size_ = 0; }

  bool empty() { return size_ == 0; }

  Move operator[](int index) { return data[index]; }

  int size() { return size_; }

  int begin() { return 0; };

  void erase(int index) {
    // move items down
    // 0, 1, 2, 3...10
    memmove(data + index, data + index + 1, (N - index - 1) * sizeof(Move));
    size_--;
  }

  void insert(int index, Move mv) {
    // move items up
    // 0, 1, 2, 3,...10
    memmove(data + index + 1, data + index, (N - index - 1) * sizeof(Move));
    data[index] = mv;
    size_++;
  }
};

#endif
