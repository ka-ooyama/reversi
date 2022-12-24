#ifndef RESULT_H
#define RESULT_H

struct CResult {
   public:
    CResult() {}
    virtual ~CResult() {}

    void set(const ulong board[])
    {
        size_t board0 = std::bitset<64>(board[0]).count();
        size_t board1 = std::bitset<64>(board[1]).count();

        high = low = (int)board0 - (int)board1;

        if (board0 > board1) {
            win = 1;
        } else if (board0 < board1) {
            lose = 1;
        } else {
            draw = 1;
        }
    }

    void marge(const CResult& result)
    {
        high = std::max<int>(high, result.high);
        low = std::min<int>(low, result.low);
        win += result.win;
        lose += result.lose;
        draw += result.draw;
    }

    void print(void) const
    {
        printf("win(%8d) lose(%8d) draw(%8d) high(%+3d) low(%+3d)\n", win, lose,
               draw, high, low);
    }

   private:
    int high = INT_MIN;
    int low = INT_MAX;
    int win = 0;
    int lose = 0;
    int draw = 0;
};
#endif  // RESULT_H
