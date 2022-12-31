#ifndef RESULT_H
#define RESULT_H

struct CResult {
public:
    //CResult(int8_t val)
    //    : evaluation_value_(val)
    //{
    //}
    CResult(int player)
        : evaluation_value_(player == 0 ? alpha_default() : beta_default())
    {
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

    void set(const uint64_t board[])
    {
        size_t t0 = std::bitset<64>(board[0]).count();
        size_t t1 = std::bitset<64>(board[1]).count();

        evaluation_value_ = (int8_t)(
            std::bitset<64>(board[0]).count() -
            std::bitset<64>(board[1]).count());
    }

    void marge(int player, const CResult& result)
    {
        if (evaluation_value_ == INT8_MIN ||
            (player == 0 && evaluation_value_ < result.evaluation_value_) ||
            (player != 0 && evaluation_value_ > result.evaluation_value_)) {
            evaluation_value_ = result.evaluation_value_;
            memcpy(choice, result.choice, sizeof(choice));
        }
    }

    void print(int progress) const
    {
        printf("progress(%3d%%) evaluation_value(%d)\n", progress, evaluation_value_);
        for (int i = 0; i < 32; i++)
        {
            printf("%d ", choice[i]);
        }
        printf("\n");
    }

    static int8_t evaluation_value_default(void)
    {
        //return -4;    // 6x6
        //return +16;   // 4x6
        return INT8_MIN;
    }

    static int8_t alpha_default(void)
    {
        return INT8_MIN;
    }

    static int8_t beta_default(void)
    {
        return INT8_MAX;
    }

    int8_t evaluation_value(void) const { return evaluation_value_; }
    void select(const int8_t evaluation_value, const int hierarchy, const int8_t bit)
    {
        evaluation_value_ = evaluation_value;
        choice[hierarchy] = bit;
    }

private:
    int8_t evaluation_value_ = INT8_MIN;
    uint8_t choice[32] = {};
};
#endif  // RESULT_H
