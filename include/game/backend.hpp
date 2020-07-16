#ifndef DS_HPP
#define DS_HPP

#include <array>
#include <vector>

#include <game/move.hpp>

struct LazyMovegen {
  std::array<int, 64> srcList;
  int srcIndex;
  int numSrcs;

  std::array<int, 64> currDests;
  int currDestIndex;
  int numCurrDests;
  bool _hasNext;


  bool hasNext() { return _hasNext; }

  LazyMovegen(u64 srcMap, std::array<u64, 64> &attackMap) {
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
    if (!hasNext()) { throw; }
    srcout = srcList[srcIndex];
    destout = currDests[currDestIndex];
    currDestIndex += 1;
    if (currDestIndex == numCurrDests) {
      srcIndex += 1;
      if (srcIndex == numSrcs) {
        _hasNext = false;
      } else {
        //load new
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
  Move mv;
};

struct KillerTable {
  std::array<std::array<std::array<u64, 2>, 3>, 20> table;

  KillerTable() { clear(); }

  void clear() {
    for (int i = 0; i < 20; i++) {
      for (int k = 0; k < 3; k++) {
        table[i][k][0] = 0;
        table[i][k][1] = 0;
      }
    }
  }
  /*
  void insert(Move &mv, int ply) {
      for (int k = 0; k < 3; k++) {
          table[ply][k][0] = 0;
          table[ply][k][1] = 0;
      }
  }
  */
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

  BoardStateNode peekNodeAt(int index) { return _data[index]; }

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

  void push(int *data, Move mv) {
    BoardStateNode node;
    for (size_t i = 0; i < BOARD_STATE_ENTROPY; i++) {
      node.data[i] = data[i];
    }
    node.mv = mv;
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

#endif
