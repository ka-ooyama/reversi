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
////////////////////////////////////////////

// CPUの並列度（△コア，〇スレッドの〇）
uint32_t hardware_concurrency = std::thread::hardware_concurrency();

// 行列(x,y)
const int rows = 6;
const int columns = 6;

const int hierarchy_single = 5;
const int hierarchy_cached = hierarchy_single + 8;
//const int hierarchy_cached = 0;

const uint32_t worker_threads_num = std::clamp<uint32_t>(32, 0, hardware_concurrency);

const int number_of_trials = 1;

#define USE_SYMMETRY    true

// x, y から通し番号を得る
int coordinateToIndex(const int x, const int y) { return y * 8 + x; }

#include "bit_util.h"
#include "node.h"
#include "result.h"

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

concurrent_unordered_map<std::pair<uint64_t, uint64_t>, CResult> result_cache[2];

std::mutex mutex;
std::vector<CNode*> jobs;
uint32_t initial_jobs_num = 1;      // 総job数
std::vector<std::thread> threads;   // ワーカースレッド
//std::atomic<uint64_t> final_num{ 0 };

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
        CResult result = simulationSingle(node->boardPointer(), node->player(), node->hierarchy());

        //printf("%3d/%3d ", worker_id, initial_jobs_num);
        result.print((int)(worker_id * 100.0f / initial_jobs_num));
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

    {
        MyTimer myTimer;

        for (int i = 0; i < number_of_trials; i++)
        {
            result_cache[0].clear();
            result_cache[1].clear();

            CNode::jobs_[0].clear();
            CNode::jobs_[1].clear();

            CNode::nodes_[0].clear();
            CNode::nodes_[1].clear();

            jobs.clear();

            if (hierarchy_single == 0) {
                CResult result = simulationSingle(board, (int)player, 0);
                result.print(0);
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
                printf("%3d/%3d \n", 0, initial_jobs_num);

                for (uint32_t i = 0; i < worker_threads_num; i++) {
                    threads.emplace_back(std::thread(worker));
                }

                for (auto&& th : threads) {
                    th.join();
                }

                threads.clear();
            }

            //printf("final %d\n", final_num.load());
            printf("result_cache %llu\n", result_cache[0].size() + result_cache[1].size());
        }
    }

    return 0;
}

void simulationPush(CNode* node)
{
    const uint64_t* board = node->boardPointer();
    int player = node->player();

    uint64_t legalBoard = makeLegalBoard(board, player);

    if (legalBoard != 0ull) {
        const int opponent = player ^ 1;
        size_t legalBoardNum = std::bitset<64>(legalBoard).count();
        bool is_push =
            (node->hierarchy() + 1) >= hierarchy_single || legalBoardNum == 1;
        uint64_t m = legalBoard;
        int bit;
        while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
            uint64_t temp_board[2] = { board[0], board[1] };
            reverse(1ull << bit, temp_board, player);
            CNode* child = node->addChild(temp_board, opponent, is_push);
            m &= ~(1ull << bit);
        }

        if (!is_push) {
            for (std::vector<CNode*>::const_iterator i =
                node->getChildren().begin();
                i != node->getChildren().end(); i++) {
                CNode* child = *i;
                simulationPush(child);
            }
        }
    } else {
        player ^= 1;

        legalBoard = makeLegalBoard(board, player);

        if (legalBoard != 0ull) {
            const int opponent = player ^ 1;
            size_t legalBoardNum = std::bitset<64>(legalBoard).count();
            bool is_push =
                (node->hierarchy() + 1) >= hierarchy_single || legalBoardNum == 1;
            uint64_t m = legalBoard;
            int bit;
            while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
                uint64_t temp_board[2] = { board[0], board[1] };
                reverse(1ull << bit, temp_board, player);
                CNode* child = node->addChild(temp_board, opponent, is_push);
                m &= ~(1ull << bit);
            }

            if (!is_push) {
                for (std::vector<CNode*>::const_iterator i =
                    node->getChildren().begin();
                    i != node->getChildren().end(); i++) {
                    CNode* child = *i;
                    simulationPush(child);
                }
            }
        } else {
            CNode* child = node->addChild(board, player ^ 1, true);
        }
    }
}

CResult simulationSingle(const uint64_t board[], int player, const int hierarchy)
{
    if (hierarchy <= hierarchy_cached) {
#if USE_SYMMETRY
        std::pair<uint64_t, uint64_t> b[8];
        board_symmetry(board, b);
        for (int i = 0; i < 8; i++) {
            auto it = result_cache[player].find(b[i]);
            if (it != result_cache[player].end()) {
                CResult result(it->second);
                return result;
            }
        }
#else
        auto it = result_cache[player].find(std::make_pair(board[0], board[1]));
        if (it != result_cache[player].end()) {
            CResult result(it->second);
            return result;
        }
#endif
    }

    CResult result;

    uint64_t legalBoard = makeLegalBoard(board, player);

    if (legalBoard != 0ull) {
        const int opponent = player ^ 1;
        uint64_t m = legalBoard;
        int bit;
        while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
            uint64_t temp_board[2] = { board[0], board[1] };
            reverse(1ull << bit, temp_board, player);
            result.marge(simulationSingle(temp_board, opponent, hierarchy + 1));
            m &= ~(1ull << bit);
        }
    } else {
        player ^= 1;

        legalBoard = makeLegalBoard(board, player);

        if (legalBoard != 0ull) {
            const int opponent = player ^ 1;
            uint64_t m = legalBoard;
            int bit;
            while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
                uint64_t temp_board[2] = { board[0], board[1] };
                reverse(1ull << bit, temp_board, player);
                result.marge(simulationSingle(temp_board, opponent, hierarchy + 1));
                m &= ~(1ull << bit);
            }
        } else {
            //final_num++;
            result.set(board);
        }
    }

    if (hierarchy <= hierarchy_cached) {
        result_cache[player][{board[0], board[1]}] = result;
    }

    return result;
}

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
