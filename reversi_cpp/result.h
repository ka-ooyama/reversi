#ifndef RESULT_H
#define RESULT_H

struct CResult {
public:
    CResult(int8_t val)
        : evaluation_value_(val)
    {}
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
        //if (s_evaluation_value_ == INT8_MIN ||
        //    (player == 0 && s_evaluation_value_ < result.evaluation_value_) ||
        //    (player != 0 && s_evaluation_value_ > result.evaluation_value_)) {
        //    s_evaluation_value_ = result.evaluation_value_;
        //}
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

    static int8_t cutline(int player)
    {
        //return player == 0 ? INT8_MAX : INT8_MIN;
        return INT8_MIN;
    }

    int8_t evaluation_value(void) const { return evaluation_value_; }
    //static int8_t s_evaluation_value(void) { return s_evaluation_value_; }

private:
    static int8_t s_evaluation_value_;
    int8_t evaluation_value_ = INT8_MIN;
    uint8_t choice[32] = {};
};
//int8_t CResult::s_evaluation_value_ = INT8_MIN;
#endif  // RESULT_H
