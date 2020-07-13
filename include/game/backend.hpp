#ifndef DS_HPP
#define DS_HPP

#include <vector>

#include <game/move.hpp>


struct PieceIndexTuple { 
    //representing a bitboard index (piece) and an indexed location.
    PieceType piece;
    int index;
    PieceIndexTuple(PieceType p, int l) {
        piece = p;
        index = l;
    }
};

struct BoardStateNode {
    int data[BOARD_STATE_ENTROPY];
    Move mv;
};

struct PseudoLegalData {
    std::vector<PieceType> moverBuffer[2];
    std::vector<u64> srcBuffer[2];
    std::vector<u64> destBuffer[2];

    u64 pieceAttacks[12]; //pseudolegal attack moves
    u64 pieceMoves[12];

    PseudoLegalData(
        std::vector<PieceType> mbw,
        std::vector<PieceType> mbb,
        std::vector<u64> sbw, 
        std::vector<u64> sbb, 
        std::vector<u64> dbw, 
        std::vector<u64> dbb, 
        u64* pa, u64* pm) {
        moverBuffer[White].assign(mbw.begin(), mbw.end());
        moverBuffer[Black].assign(mbb.begin(), mbb.end());
        srcBuffer[White].assign(sbw.begin(), sbw.end());
        srcBuffer[Black].assign(sbb.begin(), sbb.end());
        destBuffer[White].assign(dbw.begin(), dbw.end());
        destBuffer[Black].assign(dbb.begin(), dbb.end());
        for (int i = 0; i < 12; i++) {
            pieceAttacks[i] = pa[i];
            pieceMoves[i] = pm[i];
        }
    }
};

class BoardStateStack
{
    private:
        size_t _index;
        std::vector<BoardStateNode> _data;

    public:
        BoardStateStack() {
            _index = 0;
        }

        Move peekAt(int index) {
            return _data[index].mv;
        }

        void clear() {
            _index = 0;
            _data.clear();
        }
        
        BoardStateNode& peek() {
            if (_index == 0) {
                debugLog("pop from empty move stack");
                throw;
            }
            return _data.back();
        }

        void push(int* data, Move mv) {
            BoardStateNode node;
            for (size_t i = 0; i < BOARD_STATE_ENTROPY; i++) {
                node.data[i] = data[i];
            }
            node.mv = mv;
            _data.push_back(node);
            _index++;
        };

        bool canPop() {
            return _index > 0;
        }

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
