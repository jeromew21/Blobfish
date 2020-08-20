import random
import math

X = 1
O = -1
E = None

WINS = ((0, 1, 2), 
        (3, 4, 5), 
        (6, 7, 8),
        (0, 3, 6), 
        (1, 4, 7), 
        (2, 5, 8),
        (0, 4, 8), 
        (2, 4, 6))

node_count = 0

def to_str(s):
    if s == X:
        return "X"
    elif s == O:
        return "O"
    return " "

C = 1.414
def uct(w, n, N):
    return (w / n) + C * (math.sqrt(math.log(N) / n))

def rolloutPolicy(node):
    for child in node.children:
        if child.isTerminal() and child.winner() is not None:
            return child #play the winning move
    return random.choice(node.children)

class Node(object):
    def __init__(self, parent=None, mv=None):
        global node_count
        node_count += 1

        self.mv = mv
        
        self.q = 0
        self.n = 0
        
        self.parent = parent
        self._children = []
        
        if parent is not None:
            self.position = parent.position[:]
        else:
            self.position = [None for _ in range(9)]
        
        if mv is not None:
            self.position[mv[0]] = mv[1] #make move
            self.player = mv[1] #flip the thingy
        else:
            self.player = O
        
        if not self.isTerminal():
            for i in range(9):
                if self.position[i] is None:
                    self._children.append(Node(self, (i, -1*self.player)))
    
    def childFromSquare(self, i):
        if self.position[i] is not None or i < 0 or i > 8:
            raise Exception("illegal move")
        for node in self.children:
            if node.mv[0] == i:
                return node

    def winner(self):
        if self.position.count(None) == 0:
            return None #tie
        for i, j, k in WINS:
            if self.position[i] is not None:
                if self.position[i] == self.position[j] == self.position[k]:
                    return self.position[i] #winner
        return None #still playing
    
    def isTerminal(self):
        return self.winner() != None or self.position.count(None) == 0
    
    @property
    def children(self):
        return self._children
    
    def backprop(self, pr):
        node = self
        i = 0
        while node is not None:
            node.n += 1
            if i % 2 == 0:
                if pr == 1:
                    node.q += 1
            else:
                if pr ==-1:
                    node.q += 1
            i += 1
            node = node.parent
    
    def dump(self, showNums=False):
        fmt = tuple(to_str(i) for i in self.position)
        if showNums:
            fmt = tuple((i if fmt[i] == " " else fmt[i]) for i in range(9))
        print("""
{0}|{1}|{2}
-----
{3}|{4}|{5}
-----
{6}|{7}|{8}
    """.format(*fmt))
    
    def playout(self):
        #go to a random child until a winnder is reached
        node = self
        while not node.isTerminal():
            node = rolloutPolicy(node)
        return 1 if node.winner() == self.player else 0
    
    def isLeaf(self):
        if self.isTerminal():
            return True
        for node in self.children:
            if node.n == 0:
                return True #Has unvisited child...
        return False
    
    def selectLeaf(self):
        node = self
        while not node.isLeaf():
            node = node.bestChildUCT()
        return node
    
    def bestChildUCT(self):
        N = self.n if self.parent is None else self.parent.n
        return max(self.children, key=lambda node: uct(node.q, node.n, N))
    
    def bestChild(self):
        return max(self.children, key=lambda node: node.q / node.n)

def mcts(root, iterations):
    print("Running {} iterations".format(iterations))
    flag = False
    for _ in range(iterations):
        leaf = root.selectLeaf()
        if leaf.isTerminal() or flag:
            flag = True
            leaf = root
        child = min(leaf.children, key=lambda node: node.n)
        child.backprop(child.playout())

    for node in root.children:
        print("{}: {}/{}".format(node.mv[0], node.q, node.n))

if __name__ == "__main__":
    print("Loading nodes into memory...")
    root = Node()
    print(node_count)
    iterations = 10000
    mcts(root, iterations)
    root.dump()
    human = O
    flag = False
    while True:
        if human in (X, O) and (human == X or flag):
            mv = int(input("Play a move: "))
            root = root.childFromSquare(mv)
            root.dump()
            if root.isTerminal():
                break
        mcts(root, iterations)
        if flag:
            root = root.bestChild()
        else:
            root = random.choice(root.children) #pick random opening
        root.dump()
        if root.isTerminal():
            break
        flag = True
    print("Done. releasing memory...")
    

    