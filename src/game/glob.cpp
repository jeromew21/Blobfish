#include <game/glob.hpp>
#include <fstream>

std::uniform_int_distribution<std::mt19937::result_type> udist(0, 100);
std::mt19937 rng;

void debugLog(const std::string& f) {
    std::ofstream fileout;

    fileout.open("debuglog.txt", std::ios_base::app);
    fileout << f << "\n";
    std::cout << "info string " << f << std::endl;
    fileout.close();
}

void sendCommand(const std::string& cmd) {
    debugLog("SEND " + cmd);
    std::cout << cmd << std::endl;
}

std::string squareName(u64 square) {
    return FILE_NAMES[u64ToCol(square)] + RANK_NAMES[u64ToRow(square)];
}

std::string squareName(int square) {
    return squareName(u64FromIndex(square));
}

int indexFromSquareName(std::string alg) {
    int col, row;
    if (alg[0] == 'a') {
        col = 0;
    } else if (alg[0] == 'b') {
        col = 1;
    } else if (alg[0] == 'c') {
        col = 2;
    } else if (alg[0] == 'd') {
        col = 3;
    } else if (alg[0] == 'e') {
        col = 4;
    } else if (alg[0] == 'f') {
        col = 5;
    } else if (alg[0] == 'g') {
        col = 6;
    } else {
        col = 7;
    }

    if (alg[1] == '1') {
        row = 0;
    } else if (alg[1] == '2') {
        row = 1;
    } else if (alg[1] == '3') {
        row = 2;
    } else if (alg[1] == '4') {
        row = 3;
    } else if (alg[1] == '5') {
        row = 4;
    } else if (alg[1] == '6') {
        row = 5;
    } else if (alg[1] == '7') {
        row = 6;
    } else {
        row = 7;
    }

    return row*8 + col;
}

std::string statusToString(BoardStatus bs, bool concise) {
    if (concise) {
        switch (bs)
        {
            case BoardStatus::Playing:
                return "";
            case BoardStatus::WhiteWin:
                return "1-0";
            case BoardStatus::BlackWin:
                return "0-1";
            case BoardStatus::Stalemate:
            case BoardStatus::Draw:
                return "1/2-1/2";
            default:
                throw;
        }
    } else {
        switch (bs)
        {
            case BoardStatus::Playing:
                return "Playing";
            case BoardStatus::WhiteWin:
                return "White Wins";
            case BoardStatus::BlackWin:
                return "Black Wins";
            case BoardStatus::Draw:
                return "Draw";
            case BoardStatus::Stalemate:
                return "Stalemate";
            default:
                throw;
        }
    }
}

void srand100(int seed) {
    std::mt19937::result_type const seedval = seed; // get this from somewhere
    rng.seed(seedval);
}

int rand100() {
  std::mt19937::result_type result = udist(rng);
  return result;
}

std::string colorToString(Color c) {
    if (c == White) {
        return "White";
    } else if (c == Black) {
        return "Black";
    } else {
        throw;
    }
}

//LSB (rightmost, uppermost)
int bitscanForward(u64 x) { //checked, should work
    return __builtin_ffsll(x) - 1;
}

//MSB (leftmost, uppermost)
int bitscanReverse(u64 x) {
    return 63 - __builtin_clzll(x);
}

int hadd(u64 x) { //checked, should work
   int count = 0;
   while (x) {
       count++;
       x &= x - 1; // reset LS1B
   }
   return count;
}

int max(int i1, int i2) {
    if (i1 > i2) {
        return i1;
    } else {
        return i2;
    }
}

int min(int i1, int i2) {
    if (i1 < i2) {
        return i1;
    } else {
        return i2;
    }
}

std::array<u64, 64> bitscanAll(u64 x, int &outsize) {
    std::array<u64, 64> result;
    int count = 0;
    u64 mask = 1;
    int i = 0;
    while (i < 64) {
        u64 test = x & mask;
        if (test) {
            result[count] = mask;
            count++;
        }
        mask = mask << 1;
        i++;
    }
    outsize = count;

    return result;
}

std::vector<u64> bitscanAll(u64 x) {
    std::vector<u64> result;
    u64 mask = 1;
    int i = 0;
    while (i < 64) {
        u64 test = x & mask;
        if (test) {
            result.push_back(mask);
        }
        mask = mask << 1;
        i++;
    }
    return result;
}

void dump64(u64 x) { //Checked, should work
    std::cout << "\n";
    int arr[64];
    int i = 0;
    while (i < 64) {
        arr[i] = x & 1;
        x = x >> 1;
        i++;
    }
    for (int i = 7; i >= 0; i--) {
        for (int k = 0; k < 8; k++) {
            std::cout << arr[i*8 + k];
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}


bool inBounds(int y, int x) {
    return (y >= 0 && y < 8) && (x >= 0 && x < 8);
}

int u64ToRow(u64 space) {
    return bitscanForward(space) / 8;
}

int u64ToCol(u64 space) {
    return bitscanForward(space) % 8;
}

int u64ToIndex(u64 space) {
    return bitscanForward(space);
}

u64 u64FromPair(int r, int c) {
    int shift = r*8 + c; //flatten index
    return u64FromIndex(shift);
}

u64 u64FromIndex(int i) { //fixed, should work
    return ((u64) 1) << i;
}

std::string yesorno(bool b) {
    return b ? "yes" : "no";
}

std::vector<std::string> tokenize(std::string instring) {
    std::istringstream iss(instring);
    std::vector<std::string> tokens;
    copy(std::istream_iterator<std::string>(iss),
     std::istream_iterator<std::string>(),
     back_inserter(tokens));
    return tokens;
}
