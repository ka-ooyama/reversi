#ifndef RESULT_H
#define RESULT_H

#include <memory.h>

struct CResult {
public:
    //CResult(int8_t val)
    //    : evaluation_value_(val)
    //{
    //}
    CResult(const int player, const int hierarchy, const int bit)
        //: evaluation_value_(player == 0 ? alpha_default(player) : beta_default(player))
        : evaluation_value_(alpha_default(player))
    {
        choice[hierarchy] = bit;
    }
    CResult() {}
    virtual ~CResult() {}

    //CResult operator-()
    //{
    //    evaluation_value = -evaluation_value;
    //    return *this;
    //}

    //CResult negaMax(int player)
    //{
    //    //evaluation_value = player == 0 ? -evaluation_value : evaluation_value;
    //    return *this;
    //}

    void set(const uint64_t board[], const int player)
    {
        size_t p = std::bitset<64>(board[0]).count();
        size_t o = std::bitset<64>(board[1]).count();

        if (o == 0) {
            evaluation_value_ = COLUMNS * ROWS;
        } else if (p == 0) {
            evaluation_value_ = -COLUMNS * ROWS;
        } else {
            evaluation_value_ = p - o;
        }

        if (player) {
            evaluation_value_ *= -1;
        }
    }

    //void marge(int player, const CResult& result)
    //{
    //    if (evaluation_value_ == alpha_default(player) ||
    //        evaluation_value_ == beta_default(player) ||
    //        (player == 0 && evaluation_value_ < result.evaluation_value_) ||
    //        (player != 0 && evaluation_value_ > result.evaluation_value_)) {
    //        evaluation_value_ = result.evaluation_value_;
    //        memcpy(choice, result.choice, sizeof(choice));
    //    }
    //}

    void marge(const CResult& result, const int player, const int hierarchy, int8_t& alpha, int8_t& beta)
    {
#if true
        if (alpha < -result.evaluation_value_) {
            alpha = evaluation_value_ = -result.evaluation_value_;
            memcpy(&choice[hierarchy], &result.choice[hierarchy], sizeof(choice) - hierarchy);
        }
#else
        if (player == 0 && result.evaluation_value_ > alpha) {
            alpha = evaluation_value_ = result.evaluation_value_;
            memcpy(&choice[hierarchy], &result.choice[hierarchy], sizeof(choice) - hierarchy);
        } else if (player != 0 && result.evaluation_value_ < beta) {
            beta = evaluation_value_ = result.evaluation_value_;
            memcpy(&choice[hierarchy], &result.choice[hierarchy], sizeof(choice) - hierarchy);
        }
#endif
    }

    void print(int progress) const
    {
        printf("progress(%3d%%) evaluation_value(%d)\n", progress, evaluation_value_);
        for (int i = 0; i < COLUMNS * ROWS - 4; i++)
        {
            printf("%d ", choice[i]);
        }
        printf("\n");
    }

    //static int8_t evaluation_value_default(void)
    //{
    //    //return -4;    // 6x6
    //    //return +16;   // 4x6
    //    return -INT8_MAX;
    //}

    static int8_t alpha_default(int player)
    {
        return -INT8_MAX;
        //return player == 0 ? -INT8_MAX : INT8_MAX;
    }

    static int8_t beta_default(int player)
    {
        return INT8_MAX;
        //return player == 0 ? INT8_MAX : -INT8_MAX;
    }

    int8_t evaluation_value(void) const { return evaluation_value_; }
    void select(const int8_t evaluation_value, const int hierarchy, const int8_t bit)
    {
        evaluation_value_ = evaluation_value;
        choice[hierarchy] = bit;
    }

    uint8_t bit(const int hierarchy) const { return choice[hierarchy]; }

    bool isValid(void) const { return abs(evaluation_value_) != INT8_MAX; }

private:
    int8_t evaluation_value_ = -INT8_MAX;
    uint8_t choice[COLUMNS * ROWS - 4] = {};
};
#endif  // RESULT_H
