#include <time.h>

#include <chess20.hpp>
#include <game/ai.hpp>

class UCIInterface {
private:
  bool _debug;

public:
  std::atomic<bool> _notThinking{true};
  std::atomic<bool> _stopKiller{false};
  Move bestMove;
  Board board;
  std::thread _task;

  std::thread _stopperTask;

  UCIInterface() {
    _notThinking = true;
    _stopKiller = false;
  }

  void think() {
    auto start = std::chrono::high_resolution_clock::now();
    //sendCommand("info string think() routine started");
    // iterative deepening

    int depth = 0;
    int nodeCount = 1;
    int depthLimit = SCORE_MAX;
    int bestScore = SCORE_MIN;
    bestMove = Move::NullMove();
    std::vector<MoveScore> prevScores;

    for (depth = 0; depth < depthLimit; depth++) {
      //sendCommand("info hashfull " + std::to_string(AI::getTable().ppm()));
      int score;
      // send principal variation move from previous
      Move calcMove = AI::rootMove(board, depth, _notThinking, score, bestMove,
                                   nodeCount, start, prevScores);
      if (_notThinking) {
        debugLog("search interrupted");
        // either the score is better or worse.
        if (score > bestScore) { // if we get a better score in stopped search
          bestMove = calcMove;
          bestScore = score;
        }
        break;
      }
      if (abs(score - SCORE_MAX) < 30 || abs(score - SCORE_MIN) < 30) {
        bestMove = calcMove;
        break;
      } else {               // it finishes at that layer
        bestMove = calcMove; // PV-move
        bestScore = score;
      }
    }
    sendCommand("bestmove " + moveToUCIAlgebraic(bestMove));
    //sendCommand("info string think() routine ended");
    sendCommand("info hashfull " + std::to_string(AI::getTable().ppm()));
    _stopKiller = true;
    _notThinking = true;
    AI::clearKillerTable(); //clean up
  }

  void stopThinking() {
    if (_task.joinable()) {
      _notThinking = true;
      _task.join();
    }
    _stopKiller = true;
    if (_stopperTask.joinable()) {
      _stopperTask.join();
    }
  }

  void delayStop(int msecs) {
    auto start = std::chrono::high_resolution_clock::now();
    int i = 0;
    int pad = 25; //25 ms pad
    while (true) {
      if (i % 64 == 0) {
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            stop - start); // or milliseconds
        int time = duration.count();
        if (time >= msecs-pad) {
          break;
        }
      }
      if (_stopKiller) {
        return;
      }
      i++;
    }
    _notThinking = true; // stop the other thread
  }

  void startThinking(int msecs, bool inf) {
    if (_task.joinable()) {
      stopThinking();
    }
    _notThinking = false;
    _stopKiller = false;
    //sendCommand("info string thread launched");
    _task = std::thread(&UCIInterface::think, this);
    if (!inf) {
      _stopperTask = std::thread(&UCIInterface::delayStop, this, msecs);
    }
  }

  void recieveCommand(std::string cmd) {
    debugLog(" > " + cmd);
    std::vector<std::string> tokens = tokenize(cmd);
    if (tokens.size() < 1) {
      return;
    }
    if (tokens[0] == "uci") {
      sendCommand("id name Jchess 0.1");
      sendCommand("id author Jerome Wei");
      sendCommand("option name Foo type check default false");
      sendCommand("uciok");
    } else if (tokens[0] == "debug") {
      if (tokens[1] == "on") {
        _debug = true;
      } else if (tokens[1] == "off") {
        _debug = false;
      }
    } else if (tokens[0] == "isready") {
      sendCommand("readyok");
    } else if (tokens[0] == "setoption") {

    } else if (tokens[0] == "register") {
      /*
      * setoption name  [value ]
              this is sent to the engine when the user wants to change the
      internal parameters of the engine. For the "button" type no value is
      needed. One string will be sent for each parameter and this will only be
      sent when the engine is waiting. The name of the option in  should not be
      case sensitive and can inludes spaces like also the value. The substrings
      "value" and "name" should be avoided in  and  to allow unambiguous
      parsing, for example do not use  = "draw value". Here are some strings for
      the example below: "setoption name Nullmove value true\n" "setoption name
      Selectivity value 3\n" "setoption name Style value Risky\n" "setoption
      name Clear Hash\n" "setoption name NalimovPath value
      c:\chess\tb\4;c:\chess\tb\5\n"
      */
    } else if (tokens[0] == "ucinewgame") {
      /*
      * ucinewgame
this is sent to the engine when the next search (started with "position" and
"go") will be from a different game. This can be a new game the engine should
play or a new game it should analyse but also the next position from a testsuite
with positions only. If the GUI hasn't sent a "ucinewgame" before the first
"position" command, the engine shouldn't expect any further ucinewgame commands
as the GUI is probably not supporting the ucinewgame command. So the engine
should not rely on this command even though all new GUIs should support it. As
the engine's reaction to "ucinewgame" can take some time the GUI should always
send "isready" after "ucinewgame" to wait for the engine to finish its
operation.*/
      AI::reset();
    } else if (tokens[0] == "position") {
      int j = 2;
      if (tokens[1] == "startpos") {
        board.reset();
      } else {
        if (tokens[1] == "fen") {
          std::string fenstring = "";
          for (int k = 2; k < 8; k++) {
            fenstring += tokens[k] + " ";
          }
          board.loadPosition(fenstring);
          j = 8;
        }
      }
      if ((int)tokens.size() > j) {
        if (tokens[j] == "moves") {
          // play moves
          for (int k = j + 1; k < (int)tokens.size(); k++) {
            auto mvtxt = tokens[k];
            Move mv = board.moveFromAlgebraic(mvtxt);
            board.makeMove(mv);
          }
        }
      }
      /*
         position [fen  | startpos ]  moves  ....
         set up the position described in fenstring on the internal board and
         play the moves on"bestmove " the internal chess board.
         if the game was played  from the start position the string "startpos"
         will be sent Note: no "new" command is needed. However, if this
         position is from a different game than the last position sent to the
         engine, the GUI should have sent a "ucinewgame" inbetween.
      */
    } else if (tokens[0] == "go") {
      // stopThinking();
      /*
         * go
         start calculating on the current position set up with the "position"
         command. There are a number of commands that can follow this command,
         all will be sent in the same string. If one command is not send its
         value should be interpreted as it would not influence the search.
         * searchmoves  ....
            restrict search to this moves only
            Example: After "position startpos" and "go infinite searchmoves
         e2e4 d2d4" the engine should only search the two moves e2e4 and d2d4
         in the initial position.
         * ponder
            start searching in pondering mode.
            Do not exit the search in ponder mode, even if it's mate!
            This means that the last move sent in in the position string is
         the ponder move. The engine can do what it wants to do, but after a
         "ponderhit" command it should execute the suggested move to ponder
         on. This means that the ponder move sent by the GUI can be
         interpreted as a recommendation about which move to ponder. However,
         if the engine decides to ponder on a different move, it should not
         display any mainlines as they are likely to be misinterpreted by the
         GUI because the GUI expects the engine to ponder on the suggested
         move.
         * wtime
            white has x msec left on the clock
         * btime
            black has x msec left on the clock
         * winc
            white increment per move in mseconds if x > 0
         * binc
            black increment per move in mseconds if x > 0
         * movestogo
         there are x moves to the next time control,
            this will only be sent if x > 0,
            if you don't get this and get the wtime and btime it's sudden
         death
         * depth
            search x plies only.
         * nodes
            search x nodes only,
         * mate
            search for a mate in x moves
         * movetime
            search exactly x mseconds
         * infinite
            search until the "stop" command. Do not exit the search without
         being told so in this mode!
      */
      Color color = board.turn();
      bool inf = false;
      int myTime = 0;
      bool divFlag = true;
      for (int k = 1; k < (int)tokens.size(); k++) {
        auto token = tokens[k];
        if (token == "wtime") {
          k++;
          if (color == White && myTime == 0) {
            myTime = std::stoi(tokens[k]);
          }
        } else if (token == "btime" && myTime == 0) {
          k++;
          if (color == Black) {
            myTime = std::stoi(tokens[k]);
          }
        } else if (token == "movetime") {
          k++;
          myTime = std::stoi(tokens[k]);
          divFlag = false;
        } else if (token == "infinite") {
          myTime = 10000000; // close enough
          inf = true;
        }
      }
      int t = myTime;
      if (divFlag) {
        t = ((double)myTime / 30.0);
      }
      startThinking(t, inf);
    } else if (tokens[0] == "stop") {
      /*
         stop
         stop calculating as soon as possible,
         don't forget the "bestmove" and possibly the "ponder" token when
         finishing the search
      */
      stopThinking();
    } else if (tokens[0] == "ponderhit") {
      /*
         * ponderhit
         the user has played the expected move. This will be sent if the engine
         was told to ponder on the same move the user has played. The engine
         should continue searching but switch from pondering to normal search.
      */
    } else if (tokens[0] == "quit") {
      /*
         * quit
              quit the program as soon as possible
      */
      exit(0);
    } else if (tokens[0] == "dump") {
      board.dump(true);
    } else if (tokens[0] == "unmake") {
      if (board.canUndo()) {
        board.unmakeMove();
      }
    }
  }

  ~UCIInterface() { stopThinking(); }
};

int main() {
  populateMoveCache();
  initializeZobrist();
  AI::init();
  // srand100(65634536);
  srand100(13194);

  sendCommand("info string initialized");
  {
    UCIInterface interface;
    for (std::string command; std::getline(std::cin, command);) {
      auto tokens = tokenize(command);
      if (tokens.size() > 0 && tokens[0] == "vector") {
        Board b;
        b.loadPosition(command.substr(7, command.size()));
        std::cout << b.vectorize();
      } // vectorize a FEN tool
      interface.recieveCommand(command);
    }
  }

  return 0;
}
