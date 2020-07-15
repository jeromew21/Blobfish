#include <time.h>

#include <chess20.hpp>
#include <game/ai.hpp>

class UCIInterface {
   private:
      bool _debug;
   public:
      std::atomic<bool> _notThinking{true};
      Move bestMove;
      Board board;
      std::thread _task;

      UCIInterface() {
         _notThinking = true;
      }

      void think()
      {
         //iterative deepening
         int depth = 0;
         int depthLimit = INTMAX;
         int bestScore = INTMIN;
         bestMove = board.legalMoves()[0];
         for (depth = 0; depth < depthLimit; depth++) {
            int score;
            //send principal variation move from previous
            Move calcMove = AI::rootMove(board, depth, _notThinking, score, bestMove);
            if (_notThinking) {
               
               if (!(calcMove == bestMove)) {
                  //either the score is better or worse.
                  if (score > bestScore) { //if we get a better score in stopped search
                     bestMove = calcMove;
                     bestScore = score;
                  } else {
                     //cry like a grandmaster: we are in trouble here...
                     //we don't know if the score is bad because we haven't found a better move yet
                     //also we don't know if we NEED to pick a different move because our PV move is refuted
                  }
               } //else: same move as last layer
               break;
            } 
            if (score >= INTMAX || score <= INTMIN) {
               bestMove = calcMove;
               /*
               int y = (int) ceil( (double) depth / 2.0 );
               if (score <= INTMIN) {
                  y *= -1;
               }
               sendCommand("info score mate " + std::to_string(y));
               */
               sendCommand("bestmove " + board.moveToUCIAlgebraic(bestMove));
               debugLog("RUN() reached end");
               return;
            } else { //it finishes at that layer
               bestMove = calcMove; //PV-move
               bestScore = score;
            }
         }
         sendCommand("bestmove " + board.moveToUCIAlgebraic(bestMove));
         debugLog("RUN() reached end");
      }

      void stopThinking() {
         if (!_notThinking) {
            _notThinking = true;
            _task.join();
            debugLog("thread stopped");
         }
      }

      void startThinking(int msecs, bool inf)
      {
         _notThinking = false;
         debugLog("thread launched");
         _task = std::thread(&UCIInterface::think, this);
         if (!inf) {
            std::this_thread::sleep_for(std::chrono::milliseconds(msecs));
            stopThinking();
         }
      }

      void recieveCommand(std::string cmd) {
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
	this is sent to the engine when the user wants to change the internal parameters
	of the engine. For the "button" type no value is needed.
	One string will be sent for each parameter and this will only be sent when the engine is waiting.
	The name of the option in  should not be case sensitive and can inludes spaces like also the value.
	The substrings "value" and "name" should be avoided in  and  to allow unambiguous parsing,
	for example do not use  = "draw value".
	Here are some strings for the example below:
	   "setoption name Nullmove value true\n"
      "setoption name Selectivity value 3\n"
	   "setoption name Style value Risky\n"
	   "setoption name Clear Hash\n"
	   "setoption name NalimovPath value c:\chess\tb\4;c:\chess\tb\5\n"
*/
         } else if (tokens[0] == "ucinewgame") {
            /*
            * ucinewgame
   this is sent to the engine when the next search (started with "position" and "go") will be from
   a different game. This can be a new game the engine should play or a new game it should analyse but
   also the next position from a testsuite with positions only.
   If the GUI hasn't sent a "ucinewgame" before the first "position" command, the engine shouldn't
   expect any further ucinewgame commands as the GUI is probably not supporting the ucinewgame command.
   So the engine should not rely on this command even though all new GUIs should support it.
   As the engine's reaction to "ucinewgame" can take some time the GUI should always send "isready"
   after "ucinewgame" to wait for the engine to finish its operation.*/
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
            if ((int) tokens.size() > j) {
               if (tokens[j] == "moves") {
                  //play moves
                  for (int k = j+1; k < (int) tokens.size(); k++) {
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
               if the game was played  from the start position the string "startpos" will be sent
               Note: no "new" command is needed. However, if this position is from a different game than
               the last position sent to the engine, the GUI should have sent a "ucinewgame" inbetween.
            */
         } else if (tokens[0] == "go") {
            if (_notThinking) {
            /*
               * go
               start calculating on the current position set up with the "position" command.
               There are a number of commands that can follow this command, all will be sent in the same string.
               If one command is not send its value should be interpreted as it would not influence the search.
               * searchmoves  .... 
                  restrict search to this moves only
                  Example: After "position startpos" and "go infinite searchmoves e2e4 d2d4"
                  the engine should only search the two moves e2e4 and d2d4 in the initial position.
               * ponder
                  start searching in pondering mode.
                  Do not exit the search in ponder mode, even if it's mate!
                  This means that the last move sent in in the position string is the ponder move.
                  The engine can do what it wants to do, but after a "ponderhit" command
                  it should execute the suggested move to ponder on. This means that the ponder move sent by
                  the GUI can be interpreted as a recommendation about which move to ponder. However, if the
                  engine decides to ponder on a different move, it should not display any mainlines as they are
                  likely to be misinterpreted by the GUI because the GUI expects the engine to ponder
                  on the suggested move.
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
                  if you don't get this and get the wtime and btime it's sudden death
               * depth 
                  search x plies only.
               * nodes 
                  search x nodes only,
               * mate 
                  search for a mate in x moves
               * movetime 
                  search exactly x mseconds
               * infinite
                  search until the "stop" command. Do not exit the search without being told so in this mode!
            */
               Color color = board.turn();
               bool inf = false;
               int myTime = 0;
               for (int k = 1; k < (int) tokens.size(); k++) {
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
                  } else if (token == "infinite") {
                     myTime = 10000000; //close enough
                     inf = true;
                  }
               }
               int t = ((double) myTime / 30.0);
               startThinking(t, inf);
            }
         } else if (tokens[0] == "stop") {
            /*
               stop
               stop calculating as soon as possible,
               don't forget the "bestmove" and possibly the "ponder" token when finishing the search
            */
            stopThinking();
         } else if (tokens[0] == "ponderhit") {
            /*
               * ponderhit
               the user has played the expected move. This will be sent if the engine was told to ponder on the same move
               the user has played. The engine should continue searching but switch from pondering to normal search.
            */
         } else if (tokens[0] == "quit") {
            /*
               * quit
	            quit the program as soon as possible
            */
           exit(0);
         } else if (tokens[0] == "dump") {
            board.dump(true);
         }
      }

      ~UCIInterface()
      {
         stopThinking();
      }
};

int main()
{
   populateMoveCache();
   initializeZobrist();
   AI::init();
   //srand100(65634536);
   srand100(13194);

   {
      UCIInterface interface;
      for (std::string command; std::getline(std::cin, command);) {
         interface.recieveCommand(command);
      }
   }
   

   return 0;
}
