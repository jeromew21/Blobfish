#include <game/ai.hpp>
#include <math.h>

using namespace MCTS;
std::atomic<bool> stop{false};

Node::Node() {}

Node::Node(Node *parent0, Move mv0) {
  q = 0;
  n = 0;
  mv = mv0;
  parent = parent0;
  children = nullptr;
}

void Node::genChildren(Board &board) {
  auto moves = board.legalMoves();
  int siz = moves.size();
  children = new Node[siz];
  c = siz;
  for (int i = 0; i < siz; i++) {
    Move mv = moves[i];
    children[i] = Node(this, mv);
  }
}

void Node::backprop(int playoutResult) {
  Node *node = this;
  int i = 0;
  while (node != nullptr) {
    if (i % 2 == 0) {
      if (playoutResult == 1) {
        node->q += 1;
      }
    } else {
      if (playoutResult == -1) {
        node->q += 1;
      }
    }
    node->n += 1;
    i++;
    node = node->parent;
  }
}

Node *Node::bestChildUCT() {
  // on a fully visited node, pick the best child
  float N;
  if (parent == nullptr) {
    N = n;
  } else {
    N = parent->n;
  }
  float bestUct = -1;
  Node *bestChild;
  for (int i = 0; i < c; i++) {
    Node *child = children + i;
    float uctScore = uct(child->q, child->n, N);
    if (uctScore > bestUct) {
      bestUct = uctScore;
      bestChild = child;
    }
  }
  return bestChild;
}

bool Node::isLeaf() {
  // Not Fully-expanded ie. is there any unvisited child

  // false if terminal

  for (int i = 0; i < c; i++) {
    Node *child = children + i;
    if (child->n == 0)
      return true;
  }
  return false;
}

int Node::playout(Board &board) {
  // Run a playout
  Color color = board.turn();
  int mc = 0;
  while (board.status() == BoardStatus::Playing) {
    Move mv = rolloutPolicy(board);
    board.makeMove(mv);
    mc++;
  }
  auto status = board.status();
  for (int i = 0; i < mc; i++) {
    board.unmakeMove();
  }
  if (color == White && status == BoardStatus::WhiteWin) {
    return 1;
  } else if (color == Black && status == BoardStatus::BlackWin) {
    return 1;
  } else if (color == White && status == BoardStatus::BlackWin) {
    return -1;
  } else if (color == Black && status == BoardStatus::WhiteWin) {
    return -1;
  } else {
    return 0;
  }
}

void Node::del() {
  for (int i = 0; i < c; i++) {
    Node *child = children + i;
    child->del();
  }
  delete this;
}

Node *Node::selectLeaf(Board &board, int &movesMade) {
  Node *node = this;
  while (!node->isLeaf()) {
    Node *bestChild = node->bestChildUCT();
    node = bestChild;
    board.makeMove(bestChild->mv);
    movesMade += 1;
  }
  return node;
}

Node *MCTS::MCTSearch(Board &board) {

  srand(10);

  Node *root = new Node(nullptr, Move());
  root->genChildren(board);
  int i = 0;
  while (i < 100) {
    std::cout << i << " playouts\n";
    int mc = 0;
    Node *leaf = root->selectLeaf(board, mc);

    // given a leaf->pick any unvisited child
    Node *child;
    for (int i = 0; i < leaf->c; i++) {
      child = leaf->children + i;
      if (child->n == 0) {
        break;
      }
    }
    board.makeMove(child->mv);
    child->genChildren(board);
    int pResult = child->playout(board);
    board.unmakeMove();
    for (int i = 0; i < mc; i++) {
      board.unmakeMove();
    }
    child->backprop(pResult);
    i++;
  }
  std::cout << "\n";
  for (int i = 0; i < root->c; i++) {
    Node *child = root->children + i;
    std::cout << child->mv.moveToUCIAlgebraic() << " " << child->q << "/"
              << child->n << "\n";
  }

  return root;
}

float MCTS::uct(float w, float n, float N) {
  float c = 1.414;
  return (w / n) + c * (sqrt(log(N) / n));
}

Move MCTS::rolloutPolicy(Board &board) {
  // fast pick best move
  int bestScore = SCORE_MIN;
  auto moves = board.legalMoves();
  int count = 0;

  Move bestMove = moves[rand() % moves.size()];
  for (int i = 0; i < moves.size(); i++) {
    Move mv = moves[i];
    board.makeMove(mv);
    int score =
        -1 * quiescence(board, 1, 0, SCORE_MIN, -1 * bestScore, stop, count, 0);
    board.unmakeMove();
    if (score > bestScore && rand100() > 50) {
      bestScore = score;
      bestMove = mv;
    }
  }
  return bestMove;
}