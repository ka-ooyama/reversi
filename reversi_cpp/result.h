#ifndef RESULT_H
#define RESULT_H

struct CResult {
public:
    CResult() {}
    virtual ~CResult() {}

    void set(const uint64_t board[])
    {
        size_t board0 = std::bitset<64>(board[0]).count();
        size_t board1 = std::bitset<64>(board[1]).count();

        if (board0 > board1) {
            win = 1;
            high = low = rows * columns - (int)board1;
        } else if (board0 < board1) {
            lose = 1;
            high = low = (int)board0 - rows * columns;
        } else {
            draw = 1;
            high = low = 0;
        }
    }

    void marge(const CResult& result)
    {
        high = std::max<int8_t>(high, result.high);
        low = std::min<int8_t>(low, result.low);
        win += result.win;
        lose += result.lose;
        draw += result.draw;
    }

    void print(int progress) const
    {
        double match = (double)win + (double)lose + (double)draw;

        int win_rate = (int)(100.0 * ((double)win / match));
        int lose_rate = (int)(100.0 * ((double)lose / match));
        //int draw_rate = (int)(100.0 * ((double)draw / match));
        int draw_rate = 100 - win_rate - lose_rate;

        if (draw_rate < 0) {
            draw_rate = 0;
        }

        printf("progress(%3d%%) win(%3d%%) lose(%3d%%) draw(%3d%%) high(%+3d) low(%+3d)\n", progress, win_rate, lose_rate,
            draw_rate, high, low);
    }

private:
    int8_t high = INT8_MIN;
    int8_t low = INT8_MAX;
    uint64_t win = 0;
    uint64_t lose = 0;
    uint64_t draw = 0;
};
#endif  // RESULT_H
