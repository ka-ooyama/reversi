#ifndef __GNUC__
#include <bit>
#endif
#include <bitset>
#include <cstdint>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

typedef uint64_t ulong;

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

// 行列(x,y)
const int rows = 4;
const int columns = 5;

const int hierarchy_mt = 11;

enum ePLAYER { ePLAYER_P0 = 0, ePLAYER_P1, NUM };

// x, y から通し番号を得る
int coordinateToIndex(const int x, const int y) { return y * 8 + x; }

void simulationPush(CNode* node);
CResult simulationSingle(const ulong board[], int player);

void reverse(const ulong put_mask, ulong board[], const int player);
ulong transfer(const ulong put, const int k);
ulong makeLegalBoard(const ulong board[], const int player);

// CPUの並列度（△コア，〇スレッドの〇）
uint32_t hardware_concurrency = std::thread::hardware_concurrency();
std::mutex mutex;

ulong empty_board = 0;

#ifndef __GNUC__
int GetNumberOfTrailingZeros(uint64_t x) { return std::countr_zero(x); }
#else
#if true
int number_of_trailing_zeros_table[64];
int GetNumberOfTrailingZeros(int64_t x)
{
    if (x == 0) return 64;

    ulong y = (ulong)(x & -x);
    int i = (int)((y * 0x03F566ED27179461ull) >> 58);
    return number_of_trailing_zeros_table[i];
}
#else
int GetNumberOfTrailingZeros(uint64_t x) { return __builtin_ctzll(x); }
#endif
#endif

std::vector<CNode*> jobs;
uint32_t initial_jobs_num = 1;
std::vector<std::thread> threads;

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
        CResult result = simulationSingle(node->boardPointer(), node->player());

        // printf("%3d/%3d ", worker_id, initial_jobs_num);
        // result.print();
    }
}

int main(void)
{
    ePLAYER player = ePLAYER_P0;
    ulong board[] = { 0, 0 };

    for (int y = 0; y < columns; y++) {
        for (int x = 0; x < rows; x++) {
            int put_bit = coordinateToIndex(x, y);
            ulong put_mask = 1ull << put_bit;
            empty_board |= put_mask;
        }
    }

#ifndef __GNUC__
#else
    ulong hash = 0x03F566ED27179461ull;
    for (int i = 0; i < 64; i++) {
        number_of_trailing_zeros_table[hash >> 58] = i;
        hash <<= 1;
    }
#endif

    // 先手
    player = ePLAYER_P0;

    // 先手（白）
    board[0] = 1ul << coordinateToIndex(rows / 2 - 0, columns / 2 - 1) |
        1ul << coordinateToIndex(rows / 2 - 1, columns / 2 - 0);
    // 後手（黒）
    board[1] = 1ul << coordinateToIndex(rows / 2 - 1, columns / 2 - 1) |
        1ul << coordinateToIndex(rows / 2 - 0, columns / 2 - 0);
#if true  // 一つ打つ
    // 後手
    player = ePLAYER_P1;

    // 先手（白）
    board[0] = 1ull << coordinateToIndex(rows / 2 - 2, columns / 2 - 1) |
        1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 1) |
        1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 1) |
        1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 0);
    // 後手（黒）
    board[1] = 1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 0);
#endif

    {
        MyTimer myTimer;

        if (hierarchy_mt == 0) {
            CResult result = simulationSingle(board, (int)player);
            result.print();
        } else {
            CNode* root = new CNode(board, (int)player, 0);

            simulationPush(root);

            initial_jobs_num = (uint32_t)jobs.size();
            printf("%3d/%3d \n", 0, initial_jobs_num);

            for (uint32_t i = 0; i < hardware_concurrency - 1; i++) {
                threads.emplace_back(std::thread(worker));
            }

            for (auto&& th : threads) {
                th.join();
            }
        }
    }

    return 0;
}

void simulationPush(CNode* node)
{
    const ulong* board = node->boardPointer();
    int player = node->player();

    ulong legalBoard = makeLegalBoard(board, player);

    if (legalBoard != 0ull) {
        const int opponent = player ^ 1;
        size_t legalBoardNum = std::bitset<64>(legalBoard).count();
        bool is_push =
            (node->hierarchy() + 1) >= hierarchy_mt || legalBoardNum == 1;
        ulong m = legalBoard;
        int bit;
        while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
            ulong temp_board[2] = { board[0], board[1] };
            reverse(1ull << bit, temp_board, player);
            CNode* child = node->addChild(temp_board, opponent);
            if (is_push) {
                jobs.push_back(child);
            }
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
                (node->hierarchy() + 1) >= hierarchy_mt || legalBoardNum == 1;
            ulong m = legalBoard;
            int bit;
            while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
                ulong temp_board[2] = { board[0], board[1] };
                reverse(1ull << bit, temp_board, player);
                CNode* child = node->addChild(temp_board, opponent);
                if (is_push) {
                    jobs.push_back(child);
                }
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
            CNode* child = node->addChild(board, player ^ 1);
            jobs.push_back(child);
        }
    }
}

CResult simulationSingle(const ulong board[], int player)
{
    CResult result;

    ulong legalBoard = makeLegalBoard(board, player);

    if (legalBoard != 0ull) {
        const int opponent = player ^ 1;
        ulong m = legalBoard;
        int bit;
        while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
            ulong temp_board[2] = { board[0], board[1] };
            reverse(1ull << bit, temp_board, player);
            result.marge(simulationSingle(temp_board, opponent));
            m &= ~(1ull << bit);
        }
    } else {
        player ^= 1;

        legalBoard = makeLegalBoard(board, player);

        if (legalBoard != 0ull) {
            const int opponent = player ^ 1;
            ulong m = legalBoard;
            int bit;
            while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
                ulong temp_board[2] = { board[0], board[1] };
                reverse(1ull << bit, temp_board, player);
                result.marge(simulationSingle(temp_board, opponent));
                m &= ~(1ull << bit);
            }
        } else {
            result.set(board);
        }
    }

    return result;
}

void reverse(const ulong put_mask, ulong board[], const int player)
{
    const int opponent = player ^ 1;

    // 着手した場合のボードを生成
    ulong rev = 0;
    for (int i = 0; i < 8; i++) {
        ulong rev_ = 0;
        ulong mask = transfer(put_mask, i);
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

ulong transfer(const ulong put, const int k)
{
    switch (k) {
    case 0:  // 上
        return (put << 8) & 0xffffffffffffff00;
    case 1:  // 右上
        return (put << 7) & 0x7f7f7f7f7f7f7f00;
    case 2:  // 右
        return (put >> 1) & 0x7f7f7f7f7f7f7f7f;
    case 3:  // 右下
        return (put >> 9) & 0x007f7f7f7f7f7f7f;
    case 4:  // 下
        return (put >> 8) & 0x00ffffffffffffff;
    case 5:  // 左下
        return (put >> 7) & 0x00fefefefefefefe;
    case 6:  // 左
        return (put << 1) & 0xfefefefefefefefe;
    case 7:  // 左上
        return (put << 9) & 0xfefefefefefefe00;
    default:
        return 0;
    }
}

ulong makeLegalBoard(const ulong board[], const int player)
{
    const int opponent = player ^ 1;

    // 左右端の番人
    ulong horizontalWatchBoard = board[opponent] & 0x7e7e7e7e7e7e7e7e;
    // 上下端の番人
    ulong verticalWatchBoard = board[opponent] & 0x00FFFFFFFFFFFF00;
    // 全辺の番人
    ulong allSideWatchBoard = board[opponent] & 0x007e7e7e7e7e7e00;
    // 空きマスのみにビットが立っているボード
    ulong blankBoard = empty_board & ~(board[player] | board[opponent]);
    // 隣に相手の色があるかを一時保存する
    ulong tmp;
    // 返り値
    ulong legalBoard;

    // 8方向チェック
    //  ・一度に返せる石は最大6つ
    //  ・高速化のためにforを展開(ほぼ意味ないけどw)
    // 左
    tmp = horizontalWatchBoard & (board[player] << 1);
    tmp |= horizontalWatchBoard & (tmp << 1);
    tmp |= horizontalWatchBoard & (tmp << 1);
    tmp |= horizontalWatchBoard & (tmp << 1);
    tmp |= horizontalWatchBoard & (tmp << 1);
    tmp |= horizontalWatchBoard & (tmp << 1);
    legalBoard = blankBoard & (tmp << 1);

    // 右
    tmp = horizontalWatchBoard & (board[player] >> 1);
    tmp |= horizontalWatchBoard & (tmp >> 1);
    tmp |= horizontalWatchBoard & (tmp >> 1);
    tmp |= horizontalWatchBoard & (tmp >> 1);
    tmp |= horizontalWatchBoard & (tmp >> 1);
    tmp |= horizontalWatchBoard & (tmp >> 1);
    legalBoard |= blankBoard & (tmp >> 1);

    // 上
    tmp = verticalWatchBoard & (board[player] << 8);
    tmp |= verticalWatchBoard & (tmp << 8);
    tmp |= verticalWatchBoard & (tmp << 8);
    tmp |= verticalWatchBoard & (tmp << 8);
    tmp |= verticalWatchBoard & (tmp << 8);
    tmp |= verticalWatchBoard & (tmp << 8);
    legalBoard |= blankBoard & (tmp << 8);

    // 下
    tmp = verticalWatchBoard & (board[player] >> 8);
    tmp |= verticalWatchBoard & (tmp >> 8);
    tmp |= verticalWatchBoard & (tmp >> 8);
    tmp |= verticalWatchBoard & (tmp >> 8);
    tmp |= verticalWatchBoard & (tmp >> 8);
    tmp |= verticalWatchBoard & (tmp >> 8);
    legalBoard |= blankBoard & (tmp >> 8);

    // 右斜め上
    tmp = allSideWatchBoard & (board[player] << 7);
    tmp |= allSideWatchBoard & (tmp << 7);
    tmp |= allSideWatchBoard & (tmp << 7);
    tmp |= allSideWatchBoard & (tmp << 7);
    tmp |= allSideWatchBoard & (tmp << 7);
    tmp |= allSideWatchBoard & (tmp << 7);
    legalBoard |= blankBoard & (tmp << 7);

    // 左斜め上
    tmp = allSideWatchBoard & (board[player] << 9);
    tmp |= allSideWatchBoard & (tmp << 9);
    tmp |= allSideWatchBoard & (tmp << 9);
    tmp |= allSideWatchBoard & (tmp << 9);
    tmp |= allSideWatchBoard & (tmp << 9);
    tmp |= allSideWatchBoard & (tmp << 9);
    legalBoard |= blankBoard & (tmp << 9);

    // 右斜め下
    tmp = allSideWatchBoard & (board[player] >> 9);
    tmp |= allSideWatchBoard & (tmp >> 9);
    tmp |= allSideWatchBoard & (tmp >> 9);
    tmp |= allSideWatchBoard & (tmp >> 9);
    tmp |= allSideWatchBoard & (tmp >> 9);
    tmp |= allSideWatchBoard & (tmp >> 9);
    legalBoard |= blankBoard & (tmp >> 9);

    // 左斜め下
    tmp = allSideWatchBoard & (board[player] >> 7);
    tmp |= allSideWatchBoard & (tmp >> 7);
    tmp |= allSideWatchBoard & (tmp >> 7);
    tmp |= allSideWatchBoard & (tmp >> 7);
    tmp |= allSideWatchBoard & (tmp >> 7);
    tmp |= allSideWatchBoard & (tmp >> 7);
    legalBoard |= blankBoard & (tmp >> 7);

    return legalBoard;
}
