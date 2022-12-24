#ifndef NODE_H
#define NODE_H

class CNode
{
   public:
    CNode(const ulong board[], int player, int hierarchy)
        : player_(player), hierarchy_(hierarchy), fChildren()
    {
        board_[0] = board[0];
        board_[1] = board[1];
    }
    virtual ~CNode()
    {
        for (::std::vector<CNode*>::iterator i = fChildren.begin();
             i != fChildren.end(); i++) {
            delete *i;
        }
    }

    CNode* addChild(const ulong board[], int player)
    {
        CNode* p = new CNode(board, player, hierarchy_ + 1);
        fChildren.push_back(p);
        return p;
    }

    const ::std::vector<CNode*>& getChildren(void) const { return fChildren; }

    const ulong* boardPointer(void) const { return board_; }
    int player(void) const { return player_; }

    // const ulong playerBoard() { return board_[0]; }
    // const ulong opponentBoard() { return board_[1]; }
    int hierarchy(void) const { return hierarchy_; }

   private:
    ulong board_[2];
    int player_;
    int hierarchy_;
    std::vector<CNode*> fChildren;
};
#endif  // NODE_H
