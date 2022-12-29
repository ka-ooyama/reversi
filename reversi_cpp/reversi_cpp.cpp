#ifndef __GNUC__
#include <bit>
#endif
#include <bitset>
#include <cstdint>
#include <future>
#include <mutex>
#include <thread>
#include <vector>
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

const int hierarchy_single = HIERARCHEY_SINGLE;
const int hierarchy_cached = HIERARCHEY_CACHED;

const int number_of_trials = NUMBER_OF_TRIALS;

const uint32_t worker_threads_num = std::clamp<uint32_t>(WORKER_THREAD_MAX, 0, hardware_concurrency);

// x, y から通し番号を得る
int coordinateToIndex(const int x, const int y) { return y * 8 + x; }

#if ANALYZE_NODE_HIERARCHEY
#define ANALYZE_NODE_HIERARCHEY_NUM    32
std::mutex analyze_node_mutex;
uint64_t analyze_node_num[ANALYZE_NODE_HIERARCHEY_NUM];
uint64_t analyze_node_cut[ANALYZE_NODE_HIERARCHEY_NUM];
#endif

#include "bit_util.h"
#include "result.h"
#include "node.h"

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

void simulationPush(CNode* node);
CResult simulationSingle(const uint64_t board[], int player, const int hierarchy);

void reverse(const uint64_t put_mask, uint64_t board[], const int player);
uint64_t transfer(const uint64_t put, const int k);
uint64_t makeLegalBoard(const uint64_t board[], const int player);

concurrent_unordered_map<std::pair<uint64_t, uint64_t>, CResult> result_cache[2][36];

std::mutex mutex;
std::vector<CNode*> jobs;
uint32_t initial_jobs_num = 1;      // 総job数
std::vector<std::thread> threads;   // ワーカースレッド
#if DEBUG_PRINT
//std::atomic<uint64_t> final_num{ 0 };
#endif

void worker(void)
{
    while (!jobs.empty()) {
        int worker_id = 0;

        CNode* node;
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (jobs.empty()) {
                break;
            }

            worker_id = (int)jobs.size();

            node = jobs.back();

            jobs.pop_back();
        }
        node->Result() = simulationSingle(node->boardPointer(), node->player(), node->hierarchy());

#if DEBUG_PRINT
        int progress = (int)(worker_id * 100.0f / initial_jobs_num);
        printf("progress(%3d%%)\n", progress);
#endif
    }
}

int main(void)
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

#if false  // 一つ打つ
    // 後手
    player = ePLAYER_P0;

    // 先手（黒）
    board[0] =
        1ull << coordinateToIndex(rows / 2 - 2, columns / 2 - 1) |
        1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 1) |
        1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 0);
    // 後手（白）
    board[1] =
        1ull << coordinateToIndex(rows / 2 - 2, columns / 2 - 2) |
        1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 1) |
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
            int bit = GetNumberOfTrailingZeros(m);
            if (bit != 64) {
                reverse(1ull << bit, board, player);
            }
        }
        player = player == ePLAYER_P0 ? ePLAYER_P1 : ePLAYER_P0;
    }
#else
    uint64_t array[10] = {
        1ull << coordinateToIndex(0, 1),
        1ull << coordinateToIndex(0, 0),
        1ull << coordinateToIndex(1, 0),
        1ull << coordinateToIndex(2, 0),

        1ull << coordinateToIndex(3, 3),
        1ull << coordinateToIndex(0, 2),
        1ull << coordinateToIndex(3, 0),
        1ull << coordinateToIndex(1, 3),
        1ull << coordinateToIndex(0, 3),
        1ull << coordinateToIndex(2, 3),
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

    printf("[%d x %d]\n", columns, rows);
    printf("論理プロセッサ   %d\n", hardware_concurrency);
    printf("ワーカースレッド %d\n", worker_threads_num);
    printf("総階層数 %d\n", columns * rows - 4);
    printf("%d階層目からマルチスレッド化する\n", hierarchy_single);
    printf("%d階層目まで結果をキャッシュする\n", hierarchy_cached);
    printf("対称形を省いて最適化する %s\n", USE_SYMMETRY_OPTIMIZE ? "true" : "false");
#if DEBUG_PRINT
    printf("\n");
#endif

    {
        MyTimer myTimer;

        for (int i = 0; i < number_of_trials; i++)
        {
#if ANALYZE_NODE_HIERARCHEY
            for (int i = 0; i < ANALYZE_NODE_HIERARCHEY_NUM; i++)
            {
                analyze_node_num[i] = analyze_node_cut[i] = 0;
            }
#endif

            for (int i = 0; i < 2; i++)
            {
                for (int j = 0; j < 36; j++)
                {
                    result_cache[i][j].clear();
                }

                CNode::jobs_[i].clear();

                CNode::nodes_[i].clear();

            }

            jobs.clear();

            if (hierarchy_single == 0) {
                CResult result = simulationSingle(board, (int)player, 0);
#if DEBUG_PRINT
                result.print(0);
#endif
            } else {
                CNode* root = new CNode(board, (int)player, 0);

                simulationPush(root);

                for (int i = 0; i < 2; i++)
                {
                    for (auto it = CNode::jobs_[i].begin(), e = CNode::jobs_[i].end(); it != e; ++it) {
                        CNode* p = it->second;
                        jobs.push_back(p);
                    }
                }

                initial_jobs_num = (uint32_t)jobs.size();

                for (uint32_t i = 0; i < worker_threads_num; i++) {
                    threads.emplace_back(std::thread(worker));
                }

                for (auto&& th : threads) {
                    th.join();
                }

                CResult result = root->compute();
                result.print(0);

                threads.clear();
            }
        }
    }

    printf("\n");
    printf("予想倍率 %llu(%d^%lf)\n", scale, PRESET_HIERARCHEY, log((double)scale) / log((double)PRESET_HIERARCHEY));
    printf("総ジョブ数 %3d\n", initial_jobs_num);
    //printf("キャッシュ総数 %llu\n", result_cache[0].size() + result_cache[1].size() + CNode::nodes_[0].size() + CNode::nodes_[1].size());
#if ANALYZE_NODE_HIERARCHEY
    //printf("最終局面 %llu\n", final_num.load());
    for (int i = 0; i < 36 && i < columns * rows - 4; i++)
    {
        printf("%llu / %llu = %d%%\n", analyze_node_cut[i], analyze_node_num[i], (int)((double)analyze_node_cut[i] / (double)analyze_node_num[i] * 100.0));
    }
#endif

    return 0;
}

bool simulationPushBase(CNode* node, int player)
{
    const uint64_t* board = node->boardPointer();
    uint64_t legalBoard = makeLegalBoard(board, player);

    if (legalBoard != 0ull) {
        const int opponent = player ^ 1;
        size_t legalBoardNum = std::bitset<64>(legalBoard).count();
        bool is_push = (node->hierarchy() + 1) >= hierarchy_single;
        uint64_t m = legalBoard;
        int bit;
        while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
            uint64_t temp_board[2] = { board[0], board[1] };
            reverse(1ull << bit, temp_board, player);
            if (CNode* child = node->addChild(temp_board, opponent, is_push)) {
                if (!is_push) {
                    simulationPush(child);
                }
            }
            m &= ~(1ull << bit);
        }
        return true;
    }
    return false;
}

void simulationPush(CNode* node)
{
    const uint64_t* board = node->boardPointer();
    const int player = node->player();

    if (simulationPushBase(node, player)) {
        return;
    }

    if (simulationPushBase(node, player ^ 1)) {
        return;
    }

    node->pushResult();
}

bool simulationSingleBase(CResult* result, const uint64_t board[], int player, const int hierarchy, int8_t cutline)
{
    uint64_t legalBoard = makeLegalBoard(board, player);
    if (legalBoard != 0ull) {
        if (hierarchy <= hierarchy_cached) {
#if USE_SYMMETRY_OPTIMIZE
            for (int i = 0; i < 8; i++) {
                std::pair<uint64_t, uint64_t> b = { symmetry_naive(i, board[0]), symmetry_naive(i, board[1]) };
                auto it = result_cache[player][hierarchy].find(b);
                if (it != result_cache[player][hierarchy].end()) {
                    *result = it->second;
#if ANALYZE_NODE_HIERARCHEY
                    //final_num += result.match();
                    {
                        std::lock_guard<std::mutex> lock(analyze_node_mutex);
                        analyze_node_num[hierarchy]++;
                        analyze_node_cut[hierarchy]++;
                    }
#endif
                    return true;
                }
            }
#else
            auto it = result_cache[player][hierarchy].find(std::make_pair(board[0], board[1]));
            if (it != result_cache[player][hierarchy].end()) {
                *result = it->second;
                return true;
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
        while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
            uint64_t temp_board[2] = { board[0], board[1] };
            reverse(1ull << bit, temp_board, player);
            CResult rt = simulationSingle(temp_board, opponent, hierarchy + 1);
            result->marge(player, rt);
            if ((player == 0 && rt.evaluation_value() > cutline) ||
                (player != 0 && rt.evaluation_value() < cutline)) {
                return true;
            }
            m &= ~(1ull << bit);
        }

        if (hierarchy <= hierarchy_cached) {
            result_cache[player][hierarchy][{board[0], board[1]}] = *result;
        }

        return true;
    }
    return false;
}

#if false
CResult simulationSingle(const uint64_t board[], int player, const int hierarchy)
{
    CResult result;

    uint64_t legalBoard = makeLegalBoard(board, player);

    if (legalBoard != 0ull) {
        if (hierarchy <= hierarchy_cached) {
#if USE_SYMMETRY_OPTIMIZE
            for (int i = 0; i < 8; i++) {
                std::pair<uint64_t, uint64_t> b = { symmetry_naive(i, board[0]), symmetry_naive(i, board[1]) };
                auto it = result_cache[player][hierarchy].find(b);
                if (it != result_cache[player][hierarchy].end()) {
#if ANALYZE_NODE_HIERARCHEY
                    {
                        std::lock_guard<std::mutex> lock(analyze_node_mutex);
                        analyze_node_num[hierarchy]++;
                        analyze_node_cut[hierarchy]++;
                    }
#endif
                    return it->second;
                }
            }
#else
            auto it = result_cache[player][hierarchy].find(std::make_pair(board[0], board[1]));
            if (it != result_cache[player][hierarchy].end()) {
#if ANALYZE_NODE_HIERARCHEY
                {
                    std::lock_guard<std::mutex> lock(analyze_node_mutex);
                    analyze_node_num[hierarchy]++;
                    analyze_node_cut[hierarchy]++;
                }
#endif
                return it->second;
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
        while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
            uint64_t temp_board[2] = { board[0], board[1] };
            reverse(1ull << bit, temp_board, player);
            result.marge(player, simulationSingle(temp_board, opponent, hierarchy + 1));
            m &= ~(1ull << bit);
        }
    } else {
        int opponent = player ^ 1;


        legalBoard = makeLegalBoard(board, player);

    }

    if (simulationSingleBase(&result, board, player, hierarchy)) {
        return result;
    }

    if (simulationSingleBase(&result, board, player ^ 1, hierarchy)) {
        return result;
    }

    result.set(board);

    if (hierarchy <= hierarchy_cached) {
        result_cache[player][hierarchy][{board[0], board[1]}] = result;
    }

    return result;
}
#else
CResult simulationSingle(const uint64_t board[], int player, const int hierarchy)
{
    CResult result;

    if (simulationSingleBase(&result, board, player, hierarchy)) {
        return result;
    }

    if (simulationSingleBase(&result, board, player ^ 1, hierarchy)) {
        return result;
    }

    result.set(board);

    if (hierarchy <= hierarchy_cached) {
        result_cache[player][hierarchy][{board[0], board[1]}] = result;
    }

    return result;
}
#endif

void reverse(const uint64_t put_mask, uint64_t board[], const int player)
{
    const int opponent = player ^ 1;

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
}
