#ifndef NODE_H
#define NODE_H

class CNode
{
public:
    static std::unordered_map<std::pair<uint64_t, uint64_t>, CNode*> jobs_[2];
    //private:
    static std::unordered_map<std::pair<uint64_t, uint64_t>, CNode*> nodes_[2];

public:
    CNode(const uint64_t board[], int player, int hierarchy)
        : player_(player), hierarchy_(hierarchy), fChildren()
    {
        board_[0] = board[0];
        board_[1] = board[1];
    }
    virtual ~CNode()
    {
        for (::std::vector<CNode*>::iterator i = fChildren.begin();
            i != fChildren.end(); i++) {
            delete* i;
        }
    }

    CNode* addChild(const uint64_t board[], int player, bool is_job)
    {
        bool is_exist = false;

        const int nb_loop = USE_SYMMETRY_OPTIMIZE ? 8 : 1;

#if USE_SYMMETRY_OPTIMIZE
        std::pair<uint64_t, uint64_t> b[8];
        board_symmetry(board, b);
#else
        std::pair<uint64_t, uint64_t> b[1] = std::make_pair(board[0], board[1]);
#endif

        CNode* p = nullptr;

        if (hierarchy_ <= hierarchy_cached) {
            for (int i = 0; i < nb_loop; i++)
            {
                auto it_node = nodes_[player].find(b[i]);
                if (it_node != nodes_[player].end()) {
                    p = it_node->second;
                    is_exist = true;
                    break;
                }
            }
        }

        if (p == nullptr) {
            p = new CNode(board, player, hierarchy_ + 1);
            nodes_[player][b[0]] = p;
            if (is_job) {
                jobs_[player][b[0]] = p;
            }
        } else if (is_job) {
            bool is_find = false;
            if (hierarchy_ <= hierarchy_cached) {
                for (int i = 0; i < nb_loop; i++)
                {
                    auto it_job = jobs_[player].find(b[i]);
                    if (it_job != jobs_[player].end()) {
                        is_find = true;
                        break;
                    }
                }
            }
            if (!is_find) {
                jobs_[player][b[0]] = p;
            }
        }

        fChildren.push_back(p);

        if (hierarchy_ <= PRESET_HIERARCHEY) {
            is_exist = false;
        }

        return is_exist ? nullptr : p;
    }

    const ::std::vector<CNode*>& getChildren(void) const { return fChildren; }

    const uint64_t* boardPointer(void) const { return board_; }
    int player(void) const { return player_; }
    int hierarchy(void) const { return hierarchy_; }

    // const uint64_t playerBoard() { return board_[0]; }
    // const uint64_t opponentBoard() { return board_[1]; }

private:
    uint64_t board_[2];
    int player_;
    int hierarchy_;
    std::vector<CNode*> fChildren;
};

std::unordered_map<std::pair<uint64_t, uint64_t>, CNode*> CNode::jobs_[2];
std::unordered_map<std::pair<uint64_t, uint64_t>, CNode*> CNode::nodes_[2];

#endif  // NODE_H
