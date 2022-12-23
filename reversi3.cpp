// #include <math.h>
// #include <stddef.h>
// #include <stdio.h>
#include <unistd.h>

#include <cstdint>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

struct MyTimer {
    clock_t start;
    MyTimer() { start = clock(); }
    virtual ~MyTimer()
    {
        clock_t end = clock();
        printf("経過時間 = %fsec.\n", (double)(end - start) / CLOCKS_PER_SEC);
    }
};

typedef uint64_t UInt64;

// 行と列（本当は8x8だが6x6でテスト中）
const int rows = 4;
const int columns = 5;

const int yNum = 4;
const int xNum = 5;

enum ePLAYER { ePLAYER_P0 = 0, ePLAYER_P1, NUM };
ePLAYER player = ePLAYER_P0;

UInt64 piece[] = {0, 0};

int coordinateToIndex(int x, int y);
void thread_test0(uint32_t, bool);
void multi_thread(void (*func)(uint32_t, bool), uint32_t loop_count,
                  uint32_t concurrency, bool lock = true);
int simulation_mt(UInt64 piece_player, UInt64 piece_rybal);
int simulation(UInt64 piece_player, UInt64 piece_rybal);

bool bitTest(int x, int y, UInt64 mask);
void Place(int x, int y, UInt64 &piece_player, UInt64 &piece_rybal);
UInt64 transfer(UInt64 put, int k);
UInt64 makeLegalBoard(UInt64 piece_player, UInt64 piece_rybal);

// CPUの並列度（△コア，〇スレッドの〇）
uint32_t concurrency = std::thread::hardware_concurrency();
std::mutex mutex;
int active_thread = 0;

int main(void)
{
    // 先手
    player = ePLAYER_P0;

    // 先手（白）
    piece[0] = 1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 1) |
               1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 0);
    // 後手（黒）
    piece[1] = 1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 1) |
               1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 0);
#if true  // 一つ打つ
    // 後手
    player = ePLAYER_P1;

    // 先手（白）
    piece[0] = 1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 1) |
               1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 1) |
               1ull << coordinateToIndex(rows / 2 + 1, columns / 2 - 1) |
               1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 0);
    // 後手（黒）
    piece[1] = 1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 0);
#endif

    // multi_thread(thread_test0, 0, concurrency);

    {
        ePLAYER rybal = player == ePLAYER_P0 ? ePLAYER_P1 : ePLAYER_P0;

        MyTimer myTimer;
        // for (int i = 0; i < 32; i++) {
        //     int variation = simulation(piece[(int)player],
        //     piece[(int)rybal]);
        // }
        active_thread++;
        int variation = simulation_mt(piece[(int)player], piece[(int)rybal]);
    }

    return 0;
}

// x, y から通し番号を得る
int coordinateToIndex(int x, int y)
{
    // return y * rows + x;
    return y * 8 + x;
}

void thread_test0(uint32_t, bool)
{
    while (1) {
    }

    //  sleep(1);
}

void multi_thread(void (*func)(uint32_t, bool), uint32_t loop_count,
                  uint32_t concurrency, bool lock)
{
    // MyTimer myTimer;

    std::vector<std::thread> threads;

    for (uint32_t i = 0; i < concurrency; i++) {
        threads.emplace_back(std::thread(func, loop_count, lock));
    }

    for (auto &&thread : threads) {
        thread.join();
    }
}

int simulation_mt(UInt64 piece_player, UInt64 piece_rybal)
{
    std::vector<std::future<int>> futures;

    int num = 0;

    UInt64 legalBoard = makeLegalBoard(piece_player, piece_rybal);

    for (int y = 0; y < yNum; y++) {
        for (int x = 0; x < xNum; x++) {
            if (bitTest(x, y, legalBoard)) {
                UInt64 temp_player = piece_player;
                UInt64 temp_rybal = piece_rybal;
                Place(x, y, temp_player, temp_rybal);
                bool mt = false;
                mutex.lock();
                if (active_thread < concurrency) {
                    mt = true;
                    active_thread++;
                }
                mutex.unlock();
                if (mt) {
                    futures.emplace_back(std::async(std::launch::async,
                                                    simulation_mt, piece_player,
                                                    piece_rybal));
                } else {
                    num += simulation(temp_rybal, temp_player);
                }
            }
        }
    }

    for (auto &f : futures) {
        num += f.get();
    }

    if (num == 0) {
        UInt64 tmp = piece_player;
        piece_player = piece_rybal;
        piece_rybal = tmp;

        legalBoard = makeLegalBoard(piece_player, piece_rybal);

        for (int y = 0; y < yNum; y++) {
            for (int x = 0; x < xNum; x++) {
                if (bitTest(x, y, legalBoard)) {
                    UInt64 temp_player = piece_player;
                    UInt64 temp_rybal = piece_rybal;
                    Place(x, y, temp_player, temp_rybal);
                    bool mt = false;
                    mutex.lock();
                    if (active_thread < concurrency) {
                        mt = true;
                        active_thread++;
                    }
                    mutex.unlock();
                    if (mt) {
                        futures.emplace_back(
                            std::async(std::launch::async, simulation_mt,
                                       piece_player, piece_rybal));
                    } else {
                        num += simulation(temp_rybal, temp_player);
                    }
                }
            }
        }
    }

    for (auto &f : futures) {
        num += f.get();
    }

    return num == 0 ? 1 : num;
    // 置けるセルの数を返す
    // return GetBitCount(placeable[(int)player]);
}

int simulation(UInt64 piece_player, UInt64 piece_rybal)
{
    int num = 0;

    UInt64 legalBoard = makeLegalBoard(piece_player, piece_rybal);

    for (int y = 0; y < yNum; y++) {
        for (int x = 0; x < xNum; x++) {
            if (bitTest(x, y, legalBoard)) {
                UInt64 temp_player = piece_player;
                UInt64 temp_rybal = piece_rybal;
                Place(x, y, temp_player, temp_rybal);
                num += simulation(temp_rybal, temp_player);
            }
        }
    }

    if (num == 0) {
        UInt64 tmp = piece_player;
        piece_player = piece_rybal;
        piece_rybal = tmp;

        legalBoard = makeLegalBoard(piece_player, piece_rybal);

        for (int y = 0; y < yNum; y++) {
            for (int x = 0; x < xNum; x++) {
                if (bitTest(x, y, legalBoard)) {
                    UInt64 temp_player = piece_player;
                    UInt64 temp_rybal = piece_rybal;
                    Place(x, y, temp_player, temp_rybal);
                    num += simulation(temp_rybal, temp_player);
                }
            }
        }
    }

    return num == 0 ? 1 : num;
    // 置けるセルの数を返す
    // return GetBitCount(placeable[(int)player]);
}

bool bitTest(int x, int y, UInt64 mask)
{
    int piece_bit = coordinateToIndex(x, y);
    UInt64 piece_mask = 1ull << piece_bit;
    return (mask & piece_mask) == piece_mask;
}

void Place(int x, int y, UInt64 &piece_player, UInt64 &piece_rybal)
{
    int piece_bit = coordinateToIndex(x, y);
    UInt64 piece_mask = 1ull << piece_bit;

    // 着手した場合のボードを生成
    UInt64 rev = 0;
    for (int i = 0; i < 8; i++) {
        UInt64 rev_ = 0;
        UInt64 mask = transfer(piece_mask, i);
        while ((mask != 0) && ((mask & piece_rybal) != 0)) {
            rev_ |= mask;
            mask = transfer(mask, i);
        }
        if ((mask & piece_player) != 0) {
            rev |= rev_;
        }
    }

    // 反転する
    piece_player ^= piece_mask | rev;
    piece_rybal ^= rev;
}

UInt64 transfer(UInt64 put, int k)
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

UInt64 makeLegalBoard(UInt64 piece_player, UInt64 piece_rybal)
{
    // 左右端の番人
    UInt64 horizontalWatchBoard = piece_rybal & 0x7e7e7e7e7e7e7e7e;
    // 上下端の番人
    UInt64 verticalWatchBoard = piece_rybal & 0x00FFFFFFFFFFFF00;
    // 全辺の番人
    UInt64 allSideWatchBoard = piece_rybal & 0x007e7e7e7e7e7e00;
    // 空きマスのみにビットが立っているボード
    UInt64 blankBoard = ~(piece_player | piece_rybal);
    // 隣に相手の色があるかを一時保存する
    UInt64 tmp;
    // 返り値
    UInt64 legalBoard;

    // 8方向チェック
    //  ・一度に返せる石は最大6つ
    //  ・高速化のためにforを展開(ほぼ意味ないけどw)
    // 左
    tmp = horizontalWatchBoard & (piece_player << 1);
    tmp |= horizontalWatchBoard & (tmp << 1);
    tmp |= horizontalWatchBoard & (tmp << 1);
    tmp |= horizontalWatchBoard & (tmp << 1);
    tmp |= horizontalWatchBoard & (tmp << 1);
    tmp |= horizontalWatchBoard & (tmp << 1);
    legalBoard = blankBoard & (tmp << 1);

    // 右
    tmp = horizontalWatchBoard & (piece_player >> 1);
    tmp |= horizontalWatchBoard & (tmp >> 1);
    tmp |= horizontalWatchBoard & (tmp >> 1);
    tmp |= horizontalWatchBoard & (tmp >> 1);
    tmp |= horizontalWatchBoard & (tmp >> 1);
    tmp |= horizontalWatchBoard & (tmp >> 1);
    legalBoard |= blankBoard & (tmp >> 1);

    // 上
    tmp = verticalWatchBoard & (piece_player << 8);
    tmp |= verticalWatchBoard & (tmp << 8);
    tmp |= verticalWatchBoard & (tmp << 8);
    tmp |= verticalWatchBoard & (tmp << 8);
    tmp |= verticalWatchBoard & (tmp << 8);
    tmp |= verticalWatchBoard & (tmp << 8);
    legalBoard |= blankBoard & (tmp << 8);

    // 下
    tmp = verticalWatchBoard & (piece_player >> 8);
    tmp |= verticalWatchBoard & (tmp >> 8);
    tmp |= verticalWatchBoard & (tmp >> 8);
    tmp |= verticalWatchBoard & (tmp >> 8);
    tmp |= verticalWatchBoard & (tmp >> 8);
    tmp |= verticalWatchBoard & (tmp >> 8);
    legalBoard |= blankBoard & (tmp >> 8);

    // 右斜め上
    tmp = allSideWatchBoard & (piece_player << 7);
    tmp |= allSideWatchBoard & (tmp << 7);
    tmp |= allSideWatchBoard & (tmp << 7);
    tmp |= allSideWatchBoard & (tmp << 7);
    tmp |= allSideWatchBoard & (tmp << 7);
    tmp |= allSideWatchBoard & (tmp << 7);
    legalBoard |= blankBoard & (tmp << 7);

    // 左斜め上
    tmp = allSideWatchBoard & (piece_player << 9);
    tmp |= allSideWatchBoard & (tmp << 9);
    tmp |= allSideWatchBoard & (tmp << 9);
    tmp |= allSideWatchBoard & (tmp << 9);
    tmp |= allSideWatchBoard & (tmp << 9);
    tmp |= allSideWatchBoard & (tmp << 9);
    legalBoard |= blankBoard & (tmp << 9);

    // 右斜め下
    tmp = allSideWatchBoard & (piece_player >> 9);
    tmp |= allSideWatchBoard & (tmp >> 9);
    tmp |= allSideWatchBoard & (tmp >> 9);
    tmp |= allSideWatchBoard & (tmp >> 9);
    tmp |= allSideWatchBoard & (tmp >> 9);
    tmp |= allSideWatchBoard & (tmp >> 9);
    legalBoard |= blankBoard & (tmp >> 9);

    // 左斜め下
    tmp = allSideWatchBoard & (piece_player >> 7);
    tmp |= allSideWatchBoard & (tmp >> 7);
    tmp |= allSideWatchBoard & (tmp >> 7);
    tmp |= allSideWatchBoard & (tmp >> 7);
    tmp |= allSideWatchBoard & (tmp >> 7);
    tmp |= allSideWatchBoard & (tmp >> 7);
    legalBoard |= blankBoard & (tmp >> 7);

    return legalBoard;
}
