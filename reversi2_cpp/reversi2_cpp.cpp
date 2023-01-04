//#ifndef __GNUC__
//#include <bit>
//#endif
#include <iostream>
#include <thread>
#include <unordered_map>
#include <bitset>
#include <cstdint>
#include <algorithm>
#include <concurrent_unordered_map.h>

using namespace concurrency;

//////////////// 以下を貼る ////////////////
template<class T> size_t HashCombine(const size_t seed, const T& v)
{
    return seed ^ (std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}
/* pair用 */
template<class T, class S> struct std::hash<std::pair<T, S>> {
    size_t operator()(const std::pair<T, S>& keyval) const noexcept
    {
        return HashCombine(std::hash<T>()(keyval.first), keyval.second);
    }
};

//template<> struct std::hash<std::pair<uint64_t, uint64_t>> {
//    size_t operator()(const std::pair<uint64_t, uint64_t>& keyval) const noexcept
//    {
//        return std::hash<uint64_t>()(
//            (keyval.first & 0b0000000000000000001111110011111100110011001100110011111100111111) |
//            ((keyval.second << 18) & 0b1111110011111100110000001100000011001100110011000000000000000000) |
//            ((keyval.second << 32) & 0b0000001100000011000000000000000000000000000000000000000000000000) |
//            ((keyval.second << 4) & 0b0000000000000000000000000000000000000000000000001100000011000000));
//    }
//};
////////////////////////////////////////////

// CPUの並列度（△コア，〇スレッドの〇）
uint32_t hardware_concurrency = std::thread::hardware_concurrency();

#include "define.h"

// 行列(columns x rows)
const int columns = COLUMNS;
const int rows = ROWS;

const int hierarchy_cached = HIERARCHEY_CACHED;

const int number_of_trials = NUMBER_OF_TRIALS;

const uint32_t worker_threads_num = std::clamp<uint32_t>(WORKER_THREAD_MAX, 0, hardware_concurrency);

// x, y から通し番号を得る
int coordinateToIndex(const int x, const int y) { return y * 8 + x; }

#include "thread_pool_executor.hpp"
#include "bit_util.h"
#include "result.h"
//#include "node.h"

using namespace nodec;

concurrent::ThreadPoolExecutor executor;

struct MyTimer {
    clock_t start;
    MyTimer() { start = clock(); }
    virtual ~MyTimer()
    {
        clock_t end = clock();
        printf("経過時間 = %fsec.\n", (double)(end - start) / CLOCKS_PER_SEC);
    }
};

enum ePLAYER { ePLAYER_P0 = 0, ePLAYER_P1, NUM };

inline CResult simulationSingle(const int bit, const uint64_t const board[], const int player, const int hierarchy, const int8_t alpha, const int8_t beta);

void reverse(const int bit, uint64_t board[], const int player);

concurrent_unordered_map<std::pair<uint64_t, uint64_t>, uint8_t> result_cache[2][COLUMNS * ROWS - 4];

std::mutex jobs_counter_mutex;
uint32_t job_counter = 0;

bool isJobAvailable(void)
{
    if (job_counter < worker_threads_num) {
        std::lock_guard<std::mutex> lock(jobs_counter_mutex);
        if (job_counter < worker_threads_num) {
            job_counter++;
            return true;
        }
    }
    return false;
}

void decJobCounter(void)
{
    std::lock_guard<std::mutex> lock(jobs_counter_mutex);
    job_counter--;
}

int8_t hierarchy_min = COLUMNS * ROWS;

void hierarchy_print(const int hierarchy)
{
    if (hierarchy_min > hierarchy) {
        printf("%d is finish.\n", hierarchy);
        hierarchy_min = hierarchy;
    }
}

int main()
{
    initialize_bit_util();

    uint64_t board[] = { 0, 0 };

    // 先手
    ePLAYER player = ePLAYER_P0;

    // 先手（黒）
    board[0] =
        1ul << coordinateToIndex(rows / 2 - 0, columns / 2 - 1) |
        1ul << coordinateToIndex(rows / 2 - 1, columns / 2 - 0);
    // 後手（白）
    board[1] =
        1ul << coordinateToIndex(rows / 2 - 1, columns / 2 - 1) |
        1ul << coordinateToIndex(rows / 2 - 0, columns / 2 - 0);

#if false  // 一つ打つ
    // 後手
    player = ePLAYER_P1;

    // 先手（黒）
    board[0] =
        1ull << coordinateToIndex(rows / 2 - 2, columns / 2 - 1) |
        1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 1) |
        1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 1) |
        1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 0);
    // 後手（白）
    board[1] =
        1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 0);
#endif

    uint64_t scale = 1;

#if true
    for (int i = 0; i < PRESET_HIERARCHEY; i++)
    {
        uint64_t legalBoard = makeLegalBoard(board, player);
        if (legalBoard != 0ull) {
            const int opponent = player ^ 1;
            size_t legalBoardNum = std::bitset<64>(legalBoard).count();
            scale *= legalBoardNum;
            uint64_t m = legalBoard;
            //int bit = GetNumberOfTrailingZeros(m);
            int bit = std::countr_zero(m);
            if (bit != 64) {
                reverse(bit, board, player);
            }
        }
        player = player == ePLAYER_P0 ? ePLAYER_P1 : ePLAYER_P0;
    }
#else
    int array[10] = {
        coordinateToIndex(0, 1),
        coordinateToIndex(0, 0),
        coordinateToIndex(1, 0),
        coordinateToIndex(2, 0),

        coordinateToIndex(3, 3),
        coordinateToIndex(0, 2),
        coordinateToIndex(3, 0),
        coordinateToIndex(1, 3),
        coordinateToIndex(0, 3),
        coordinateToIndex(2, 3),
    };

    for (int i = 0; i < 5; i++)
    {
        reverse(array[i], board, player);

        printf("%d %d %d\n",
            std::bitset<64>(board[0]).count(),
            std::bitset<64>(board[1]).count(),
            std::bitset<64>(board[0]).count() + std::bitset<64>(board[1]).count()
        );

        player = player == ePLAYER_P0 ? ePLAYER_P1 : ePLAYER_P0;
    }
#endif

    {
        MyTimer myTimer;

        for (int i = 0; i < number_of_trials; i++)
        {
            for (int i = 0; i < 2; i++)
            {
                for (int j = 0; j < COLUMNS * ROWS - 4; j++)
                {
                    result_cache[i][j].clear();
                }
            }


            CResult result = simulationSingle(0, board, (int)player, 0, CResult::alpha_default((int)player), CResult::beta_default((int)player));
#if DEBUG_PRINT
            result.print(0);
        }
#endif
    }

    return 0;
}

bool simulationSingleBase(CResult* result, const uint64_t board[], const int player, const int hierarchy, int8_t alpha, int8_t beta)
{
    uint64_t legalBoard = makeLegalBoard(board, player);
    if (legalBoard != 0ull) {
        int priority_bit = INT_MAX;

        if (hierarchy <= hierarchy_cached) {
#if USE_SYMMETRY_OPTIMIZE
            for (int i = 0; i < 8; i++) {
                std::pair<uint64_t, uint64_t> b = { symmetry_naive(i, board[0]), symmetry_naive(i, board[1]) };
                auto it = result_cache[player][hierarchy].find(b);
                if (it != result_cache[player][hierarchy].end()) {
                    priority_bit = it->second;
                    break;
                    //*result = it->second;
#if ANALYZE_NODE_HIERARCHEY
                    //final_num += result.match();
                    {
                        std::lock_guard<std::mutex> lock(analyze_node_mutex);
                        analyze_node_num[hierarchy]++;
                        analyze_node_cut[hierarchy]++;
                    }
#endif
                    //return true;
                }
            }
#else
            auto it = result_cache[player][hierarchy].find(std::make_pair(board[0], board[1]));
            if (it != result_cache[player][hierarchy].end()) {
                priority_bit = it->second;
                //return true;
            }
#endif
#if ANALYZE_NODE_HIERARCHEY
            {
                std::lock_guard<std::mutex> lock(analyze_node_mutex);
                analyze_node_num[hierarchy]++;
            }
#endif
        }

        const int opponent = player ^ 1;
        uint64_t m = legalBoard;
        int bit;
        std::vector<std::future<CResult>> threads;
        for (int i = 0;
#if ALPHA_BETA
        (alpha < beta) &&
#endif
            (((bit = priority_bit) != INT_MAX) || ((bit = GetNumberOfTrailingZeros(m)) != 64)); i++) {
            uint64_t temp_board[2] = { board[0], board[1] };
            reverse(bit, temp_board, player);
            if (i == 0 || !isJobAvailable() || COLUMNS * ROWS - 4 - HIERARCHEY_SINGLE <= hierarchy) {
                CResult rt = simulationSingle(bit, temp_board, opponent, hierarchy + 1, alpha, beta);
#if false
                if (a < rt.evaluation_value()) {
                    a = rt.evaluation_value();
                    result->select(a, hierarchy, bit);
                }
#else
                if (player == 0 && rt.evaluation_value() > alpha) {
                    alpha = rt.evaluation_value();
                    result->marge(rt, hierarchy + 1);
                } else if (player != 0 && rt.evaluation_value() < beta) {
                    beta = rt.evaluation_value();
                    result->marge(rt, hierarchy + 1);
                }
#endif
            } else {
                threads.push_back(executor.submit(simulationSingle, std::move(bit), std::move(temp_board), std::move(opponent), hierarchy + 1, std::move(alpha), std::move(beta)));
            }
            priority_bit = INT_MAX;
            m &= ~(1ull << bit);
        }

        //printf("wait %d\n", hierarchy);

#if true
        for (auto& val : threads) {
            CResult rt = val.get();
            decJobCounter();
#if false
            if (a < rt.evaluation_value()) {
                a = rt.evaluation_value();
                result->select(a, hierarchy, bit);
            }
#else
            if (player == 0 && rt.evaluation_value() > alpha) {
                alpha = rt.evaluation_value();
                result->marge(rt, hierarchy + 1);
            } else if (player != 0 && rt.evaluation_value() < beta) {
                beta = rt.evaluation_value();
                result->marge(rt, hierarchy + 1);
            }
#endif
        }
#endif

        if (hierarchy <= hierarchy_cached) {
            result_cache[player][hierarchy][{board[0], board[1]}] = result->bit(hierarchy + 1);
        }

        //printf("finish %d\n", hierarchy);

        return true;
    }
    return false;
}

CResult simulationSinglePass(const int bit, const uint64_t const board[], const int player, const int hierarchy, const int8_t alpha, const int8_t beta)
{
    CResult result(player, hierarchy, bit);

    if (simulationSingleBase(&result, board, player, hierarchy, alpha, beta)) {
        hierarchy_print(hierarchy);
        return result;
    }

    result.set(board);

    hierarchy_print(hierarchy);
    return result;
}

CResult simulationSingle(const int bit, const uint64_t const board[], const int player, const int hierarchy, const int8_t alpha, const int8_t beta)
{
    CResult result(player, hierarchy, bit);

    if (COLUMNS * ROWS - 4 == hierarchy) {
        result.set(board);
        hierarchy_print(hierarchy);
        return result;
    }

    if (simulationSingleBase(&result, board, player, hierarchy, alpha, beta)) {
        hierarchy_print(hierarchy);
        return result;
    }

    return simulationSinglePass(bit, board, player ^ 1, hierarchy, alpha, beta);
}

void reverse(const int bit, uint64_t board[], const int player)
{
    const uint64_t put_mask = 1ull << bit;
    const int opponent = player ^ 1;

#if false
    // 着手した場合のボードを生成
    uint64_t rev = 0;
    for (int i = 0; i < 8; i++) {
        uint64_t rev_ = 0;
        uint64_t mask = transfer(put_mask, i);
        while ((mask != 0) && ((mask & board[opponent]) != 0)) {
            rev_ |= mask;
            mask = transfer(mask, i);
        }
        if ((mask & board[player]) != 0) {
            rev |= rev_;
        }
    }

    // 反転する
    board[player] ^= put_mask | rev;
    board[opponent] ^= rev;
#else
    __m128i result = flip(_mm_setr_epi64x(board[player], board[opponent]), std::countr_zero(put_mask));
    board[player] ^= put_mask | result.m128i_u64[player];
    board[opponent] ^= result.m128i_u64[opponent];
#endif
}
