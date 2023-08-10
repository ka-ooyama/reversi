//#pragma inline_recursion( on )
//#pragma inline_depth( 32 )

//#ifndef __GNUC__
#include <bit>
//#endif
#include <iostream>
#include <assert.h>
#include <mutex>
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

#define PRESET_HIERARCHEY		38   // (default :  0) 予め打っておく手数（最大値：総手数）
#include "define.h"

// 行列(columns x rows)
const int columns = COLUMNS;
const int rows = ROWS;

#if OPT_CACHE
const int hierarchy_cached = OPT_CACHE ? CACHED_HIERARCHEY : -1;
concurrent_unordered_map<std::pair<uint64_t, uint64_t>, uint8_t> result_cache[2][COLUMNS * ROWS - 4 + 1];
#endif

const int number_of_trials = NUMBER_OF_TRIALS;

const uint32_t worker_threads_num = std::clamp<uint32_t>(WORKER_THREAD_MAX, 0, hardware_concurrency);

#if DEBUG_PRINT && PRINT_NODES
std::atomic<uint64_t> nodes = 0;
#endif

// x, y から通し番号を得る
int coordinateToIndex(const int x, const int y)
{
    return y * 8 + x + (8 - COLUMNS) / 2 * 8 + (8 - ROWS) / 2;
}

#if CACHE_ANALYZE && OPT_CACHE && DEBUG_PRINT
#define CACHE_ANALYZE_NUM    32
std::mutex analyze_node_mutex;
uint64_t analyze_node_num[CACHE_ANALYZE_NUM];
uint64_t analyze_node_cut[CACHE_ANALYZE_NUM];
#endif

#include "thread_pool_executor.hpp"
#include "bit_util.h"
#include "result.h"
//#include "node.h"

using namespace nodec;

concurrent::ThreadPoolExecutor executor(WORKER_THREAD_MAX < hardware_concurrency ? WORKER_THREAD_MAX : hardware_concurrency);
//concurrent::ThreadPoolExecutor executor(WORKER_THREAD_MAX);

struct MyTimer {
    clock_t start;
    MyTimer() { start = clock(); }
    virtual ~MyTimer()
    {
        clock_t end = clock();
#if DEBUG_PRINT && PRINT_NODES
        printf("経過時間 = %fsec.(正確な値は PRINT_NODES false で計測すること)\n", (double)(end - start) / CLOCKS_PER_SEC);
#else
        printf("経過時間 = %fsec.\n", (double)(end - start) / CLOCKS_PER_SEC);
#endif
    }
};

enum ePLAYER { ePLAYER_P0 = 0, ePLAYER_P1, NUM };

inline CResult simulationSingle(const int bit, const board bd, const int player, const int hierarchy, const int8_t alpha, const int8_t beta, bool& is_cancel);

void reverse(const int bit, board *bd, const int player);

int8_t hierarchy_min = COLUMNS * ROWS;

void hierarchy_print(const int hierarchy)
{
#if DEBUG_PRINT
    if (hierarchy_min > hierarchy) {
        printf("%d is finish.\n", hierarchy);
        hierarchy_min = hierarchy;
    }
#endif
}

int main()
{
    initialize_bit_util();

    // 先手（黒）
    const uint64_t black =
        1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 1) |
        1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 0);
    // 後手（白）
    const uint64_t white =
        1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 1) |
        1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 0);

    board bd(black, white);

    //uint64_t board[] = { 0, 0 };

    // 先手
    ePLAYER player = ePLAYER_P0;
    int hierarchy = 0;

    // 先手（黒）
    //board[0] =
    //    1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 1) |
    //    1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 0);
    // 後手（白）
    //board[1] =
    //    1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 1) |
    //    1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 0);

    uint64_t scale = 1;

    int preset = PRESET_HIERARCHEY;

#if OPT_TETRAGONALITY
    if (COLUMNS == ROWS && (COLUMNS % 2) == 0 && (ROWS % 2) == 0) {
        preset = std::max(1, preset);
    }
#endif

    CResult result(player, hierarchy, 0);

    for (int i = 0; i < preset; i++)
    {
        uint64_t legalBoard = makeLegalBoard(bd, player);
        if (legalBoard != 0ull) {
            const int opponent = player ^ 1;
            size_t legalBoardNum = std::bitset<64>(legalBoard).count();
            scale *= legalBoardNum;
            uint64_t m = legalBoard;
            int bit = GetNumberOfTrailingZeros(m, hierarchy);
            //int bit = std::countr_zero(m);
            if (bit != 64) {
                reverse(bit, &bd, player);
                result.select(result.evaluation_value(), hierarchy, bit);
            }
        }
        player = player == ePLAYER_P0 ? ePLAYER_P1 : ePLAYER_P0;
        hierarchy++;
    }

    printf("[%d x %d]\n", columns, rows);
    printf("論理プロセッサ   %d\n", hardware_concurrency);
    printf("ワーカースレッド %d\n", worker_threads_num);
    printf("総階層数 %d\n", columns * rows - 4);
    printf("求める階層数 %d\n", TURNS);
#if OPT_CACHE
    printf("%d階層目まで結果をキャッシュする\n", hierarchy_cached);
    printf("対称形を省いて最適化する %s\n", OPT_SYMMETRY ? "true" : "false");
#endif
#if DEBUG_PRINT
    printf("\n");
#endif

    {
        MyTimer myTimer;

        for (int i = 0; i < number_of_trials; i++)
        {
#if DEBUG_PRINT && PRINT_NODES
            nodes = 0;
#endif

#if CACHE_ANALYZE && OPT_CACHE && DEBUG_PRINT
            for (int i = 0; i < CACHE_ANALYZE_NUM; i++) {
                analyze_node_num[i] = analyze_node_cut[i] = 0;
            }
#endif

#if OPT_CACHE
            for (int i = 0; i < 2; i++)
            {
                for (int j = 0; j < COLUMNS * ROWS - 4; j++)
                {
                    result_cache[i][j].clear();
                }
            }
#endif

            bool is_cancel = false;
            CResult tmp = simulationSingle(0, bd, (int)player, hierarchy, CResult::alpha_default((int)player), CResult::beta_default((int)player), is_cancel);
            int8_t alpha = CResult::alpha_default(0);
            int8_t beta = CResult::beta_default(0);
            result.marge(tmp, player, hierarchy, alpha, beta);
#if DEBUG_PRINT
            result.print(0);
#endif
#if DEBUG_PRINT && PRINT_NODES
            printf("総ノード数 %llu\n", nodes.load());
#endif
        }

#if DEBUG_PRINT
        printf("\n");
#endif
#if CACHE_ANALYZE && OPT_CACHE && DEBUG_PRINT
        printf("各階層のキャッシュヒット率（効果が少ない階層から無効にしたほうが早い）\n");
        for (int i = 0; i <= hierarchy_cached && i < COLUMNS * ROWS - 4 && i < sizeof(analyze_node_cut) / sizeof(analyze_node_cut[0]); i++) {
            printf("%2d: %llu / %llu = %d%%\n", i, analyze_node_cut[i], analyze_node_num[i], (int)((double)analyze_node_cut[i] / (double)analyze_node_num[i] * 100.0));
        }
#endif
    }

    return 0;
}

bool simulationSingleBase(CResult* result, const board bd, const int player, const int hierarchy, int8_t alpha, int8_t beta, bool& is_cancel)
{
    uint64_t legalBoard = makeLegalBoard(bd, player);

    if (legalBoard != 0ull) {
        const int opponent = player ^ 1;
        uint64_t m = legalBoard;
        int legalBoardBits = std::bitset<64>(legalBoard).count();
        int i = 0;
        int bit;

#if OPT_CACHE
        if (hierarchy <= hierarchy_cached) {
#if OPT_SYMMETRY
            for (int i = 0; i < 8; i++) {
                std::pair<uint64_t, uint64_t> b = { symmetry_naive(i, board[0]), symmetry_naive(i, board[1]) };
                auto it = result_cache[player][hierarchy].find(b);
                if (it != result_cache[player][hierarchy].end()) {
                    bit = symmetry_table[i][it->second];
                    uint64_t temp_board[2] = { b.first, b.second };
                    reverse(bit, temp_board, player);
                    CResult rt = simulationSingle(bit, temp_board, opponent, hierarchy + 1, alpha, beta);
                    result->marge(rt, player, hierarchy, alpha, beta);
#if CACHE_ANALYZE && OPT_CACHE && DEBUG_PRINT
                    {
                        std::lock_guard<std::mutex> lock(analyze_node_mutex);
                        analyze_node_num[hierarchy]++;
                        analyze_node_cut[hierarchy]++;
                    }
#endif
                    m &= ~(1ull << bit);
                    i++;
                    break;
                }
            }
#else
            auto it = result_cache[player][hierarchy].find(std::make_pair(board[0], board[1]));
            if (it != result_cache[player][hierarchy].end()) {
                bit = it->second;
                uint64_t temp_board[2] = { board[0], board[1] };
                reverse(bit, temp_board, player);
                CResult rt = simulationSingle(bit, temp_board, opponent, hierarchy + 1, alpha, beta);
                result->marge(rt, player, hierarchy, alpha, beta);
                m &= ~(1ull << bit);
                i++;
            }
#endif
#if CACHE_ANALYZE && OPT_CACHE && DEBUG_PRINT
            {
                std::lock_guard<std::mutex> lock(analyze_node_mutex);
                analyze_node_num[hierarchy]++;
            }
#endif
        }
#endif

#if WORKER_THREAD_MAX != 0
        std::vector<std::future<CResult>> threads;
        //threads.reserve(COLUMNS * ROWS - 4);
#endif
        //std::unique_ptr<bool> tmp_cancel(new bool(false));
        bool tmp_cancel = false;
        for (;
#if OPT_ALPHA_BETA
            !is_cancel && !tmp_cancel &&
            //(alpha < beta) &&
#endif
            ((bit = GetNumberOfTrailingZeros(m, hierarchy)) != 64); i++) {
            board temp_board = bd;
            reverse(bit, &temp_board, player);
#if DEBUG_PRINT && PRINT_NODES
            nodes++;
#endif
#if WORKER_THREAD_MAX != 0
            //if (i == 0 || (TURNS - SINGLE_HIERARCHEY_BTM +2) != hierarchy || !executor.lock()) {
            //if (i == 0 || /*i == legalBoardBits ||*/ hierarchy < SINGLE_HIERARCHEY_TOP || (TURNS - SINGLE_HIERARCHEY_BTM) <= hierarchy || !executor.lock()) {
            if (i % 2 == 0 || /*i == legalBoardBits ||*/ hierarchy < SINGLE_HIERARCHEY_TOP || (TURNS - SINGLE_HIERARCHEY_BTM) <= hierarchy || !executor.lock()) {
                CResult rt = simulationSingle(bit, temp_board, opponent, hierarchy + 1, alpha, beta, tmp_cancel);
                result->marge(rt, player, hierarchy, alpha, beta);
                tmp_cancel |= !(alpha < beta);
            }
            else {
                threads.push_back(executor.submit(simulationSingle, std::move(bit), std::move(temp_board), std::move(opponent), hierarchy + 1, std::move(alpha), std::move(beta), std::ref(tmp_cancel)));
                executor.unlock();
            }
#if 1
            for (auto it = threads.begin(); it != threads.end();) {
                std::future_status status = (*it).wait_for(std::chrono::seconds(0));
                if (status != std::future_status::timeout) {
                    CResult rt = (*it).get();
                    result->marge(rt, player, hierarchy, alpha, beta);
                    tmp_cancel |= !(alpha < beta);
                    it = threads.erase(it);
                }
                else {
                    ++it;
                }
            }
#endif
#else
            CResult rt = simulationSingle(bit, temp_board, opponent, hierarchy + 1, alpha, beta, tmp_cancel);
            result->marge(rt, player, hierarchy, alpha, beta);
            tmp_cancel |= !(alpha < beta);
#endif
            m &= ~(1ull << bit);
        }

#if WORKER_THREAD_MAX != 0
#if 1
        for (auto& val : threads) {
            CResult rt = val.get();
            if (!is_cancel && !tmp_cancel) {
                result->marge(rt, player, hierarchy, alpha, beta);
                tmp_cancel |= !(alpha < beta);
            }
        }
#else
        while (!threads.empty()) {
            for (auto it = threads.begin(); it != threads.end();) {
                std::future_status status = (*it).wait_for(std::chrono::seconds(0));
                if (status != std::future_status::timeout) {
                    CResult rt = (*it).get();
                    result->marge(rt, player, hierarchy, alpha, beta);
                    tmp_cancel |= !(alpha < beta);
                    it = threads.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
#endif
#endif

#if OPT_CACHE
        if (result->isValid() && hierarchy <= hierarchy_cached) {
            result_cache[player][hierarchy][{board[0], board[1]}] = result->bit(hierarchy);
        }
#endif

        return true;
    }
    return false;
}

CResult simulationSinglePass(const int bit, const board bd, const int player, const int hierarchy, const int8_t alpha, const int8_t beta, bool& is_cancel)
{
    CResult result(player, hierarchy - 1, bit);

    if (simulationSingleBase(&result, bd, player, hierarchy, alpha, beta, is_cancel)) {
        int8_t a = alpha;
        int8_t b = beta;
        int8_t score = result.evaluation_value();
        if      (player != 0 && score > a) { a = score; }
        else if (player == 0 && score < b) { b = score; }
        is_cancel |= !(a < b);
        hierarchy_print(hierarchy);
        return result;
    }

    result.set(bd);

    hierarchy_print(hierarchy);
    return result;
}

CResult simulationSingle(const int bit, const board bd, const int player, const int hierarchy, const int8_t alpha, const int8_t beta, bool& is_cancel)
{
    CResult result(player, hierarchy - 1, bit);

    if (TURNS <= hierarchy) {
        result.set(bd);
        hierarchy_print(hierarchy);
        return result;
    }

    if (simulationSingleBase(&result, bd, player, hierarchy, alpha, beta, is_cancel)) {
        int8_t a = alpha;
        int8_t b = beta;
        int8_t score = result.evaluation_value();
        if      (player != 0 && score > a) { a = score; }
        else if (player == 0 && score < b) { b = score; }
        is_cancel |= !(a < b);
        hierarchy_print(hierarchy);
        return result;
    }

    return simulationSinglePass(bit, bd, player ^ 1, hierarchy, alpha, beta, is_cancel);
}

void reverse(const int bit, board *bd, const int player)
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
    if (player == 0) {
        __m128i result = flip(*bd, bit);
        bd->data.m128i_u64[0] ^= put_mask | result.m128i_u64[0];
        bd->data.m128i_u64[1] ^=            result.m128i_u64[1];
    }
    else {
        *bd = board::reverse_board(*bd);
        __m128i result = flip(*bd, bit);
        bd->data.m128i_u64[0] ^= put_mask | result.m128i_u64[0];
        bd->data.m128i_u64[1] ^=            result.m128i_u64[1];
        *bd = board::reverse_board(*bd);
    }
    //__m128i result = flip(_mm_setr_epi64x(board[player], board[opponent]), std::countr_zero(put_mask));
    //board[player] ^= put_mask | result.m128i_u64[player];
    //board[opponent] ^= result.m128i_u64[opponent];
#endif
}
