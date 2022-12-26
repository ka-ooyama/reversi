// #include <math.h>
// #include <stddef.h>
// #include <stdio.h>
// #include <unistd.h>

// #include <mutex>
// #include <cstdint>
#include <math.h>
#include <unistd.h>

#include <mutex>
#include <thread>
#include <vector>

// typedef unsigned long long uint64_t;
// typedef uint64_t ulong;
typedef unsigned long long ulong;
// typedef unsigned long long uint32_t;

// x, y から通し番号を得る
int coordinateToIndex(int x, int y) { return y * 8 + x; }

const ulong board_mask_4x6 =
    1ull << coordinateToIndex(1, 2) | 1ull << coordinateToIndex(1, 3) |
    1ull << coordinateToIndex(1, 4) | 1ull << coordinateToIndex(1, 5) |
    1ull << coordinateToIndex(2, 2) | 1ull << coordinateToIndex(2, 3) |
    1ull << coordinateToIndex(2, 4) | 1ull << coordinateToIndex(2, 5) |
    1ull << coordinateToIndex(3, 2) | 1ull << coordinateToIndex(3, 3) |
    1ull << coordinateToIndex(3, 4) | 1ull << coordinateToIndex(3, 5) |
    1ull << coordinateToIndex(4, 2) | 1ull << coordinateToIndex(4, 3) |
    1ull << coordinateToIndex(4, 4) | 1ull << coordinateToIndex(4, 5) |
    1ull << coordinateToIndex(5, 2) | 1ull << coordinateToIndex(5, 3) |
    1ull << coordinateToIndex(5, 4) | 1ull << coordinateToIndex(5, 5) |
    1ull << coordinateToIndex(6, 2) | 1ull << coordinateToIndex(6, 3) |
    1ull << coordinateToIndex(6, 4) | 1ull << coordinateToIndex(6, 5);

const ulong board_mask_4x5 =
    1ull << coordinateToIndex(1, 2) | 1ull << coordinateToIndex(1, 3) |
    1ull << coordinateToIndex(1, 4) | 1ull << coordinateToIndex(1, 5) |
    1ull << coordinateToIndex(2, 2) | 1ull << coordinateToIndex(2, 3) |
    1ull << coordinateToIndex(2, 4) | 1ull << coordinateToIndex(2, 5) |
    1ull << coordinateToIndex(3, 2) | 1ull << coordinateToIndex(3, 3) |
    1ull << coordinateToIndex(3, 4) | 1ull << coordinateToIndex(3, 5) |
    1ull << coordinateToIndex(4, 2) | 1ull << coordinateToIndex(4, 3) |
    1ull << coordinateToIndex(4, 4) | 1ull << coordinateToIndex(4, 5) |
    1ull << coordinateToIndex(5, 2) | 1ull << coordinateToIndex(5, 3) |
    1ull << coordinateToIndex(5, 4) | 1ull << coordinateToIndex(5, 5);

const ulong board_mask_4x4 =
    1ull << coordinateToIndex(2, 2) | 1ull << coordinateToIndex(2, 3) |
    1ull << coordinateToIndex(2, 4) | 1ull << coordinateToIndex(2, 5) |
    1ull << coordinateToIndex(3, 2) | 1ull << coordinateToIndex(3, 3) |
    1ull << coordinateToIndex(3, 4) | 1ull << coordinateToIndex(3, 5) |
    1ull << coordinateToIndex(4, 2) | 1ull << coordinateToIndex(4, 3) |
    1ull << coordinateToIndex(4, 4) | 1ull << coordinateToIndex(4, 5) |
    1ull << coordinateToIndex(5, 2) | 1ull << coordinateToIndex(5, 3) |
    1ull << coordinateToIndex(5, 4) | 1ull << coordinateToIndex(5, 5);

// 行と列（本当は8x8だが6x6でテスト中）
const int rows = 8;
const int columns = 8;

const int yNum = 8;
const int xNum = 8;

// 8方向の検索用ベクトル
const int searchVec[8][2] = {{-1, -1}, {+0, -1}, {+1, -1}, {-1, +0},
                             {+1, +0}, {-1, +1}, {+0, +1}, {+1, +1}};

enum ePLAYER { ePLAYER_P0 = 0, ePLAYER_P1, NUM };
ePLAYER player = ePLAYER_P0;

ulong piece[] = {0, 0};
// ulong placeable[] = {0, 0};
ulong board_mask = 0;

int coordinateToIndex(int x, int y);
void thread_test0(uint32_t, bool);
void multi_thread(void (*func)(uint32_t, bool), uint32_t loop_count,
                  uint32_t concurrency, bool lock = true);
int simuration(ulong piece_player, ulong piece_rybal);

bool isPlaceable(int x, int y, ulong piece_player, ulong piece_rybal);
void Place(int x, int y, ulong &piece_player, ulong &piece_rybal);

struct MyTimer {
    clock_t start;
    MyTimer() { start = clock(); }
    virtual ~MyTimer()
    {
        clock_t end = clock();
        printf("経過時間 = %fsec.\n", (double)(end - start) / CLOCKS_PER_SEC);
    }
};

int main(void)
{
    // CPUの並列度（△コア，〇スレッドの〇）
    uint32_t concurrency = std::thread::hardware_concurrency();

    // 先手
    player = ePLAYER_P0;

#if false
    ulong disable_mask = 0;

    // 先手（白）
    piece[0] = 1ull << coordinateToIndex(0, 0) |
               1ull << coordinateToIndex(3, 3) |
               1ull << coordinateToIndex(3, 4);
    // 後手（黒）
    piece[1] =
        1ull << coordinateToIndex(0, 1) | 1ull << coordinateToIndex(0, 2) |
        1ull << coordinateToIndex(0, 3) | 1ull << coordinateToIndex(0, 4) |
        1ull << coordinateToIndex(0, 5) | 1ull << coordinateToIndex(1, 0) |
        1ull << coordinateToIndex(1, 1) | 1ull << coordinateToIndex(1, 2) |
        1ull << coordinateToIndex(1, 3) | 1ull << coordinateToIndex(1, 4) |
        1ull << coordinateToIndex(1, 5) | 1ull << coordinateToIndex(2, 1) |
        1ull << coordinateToIndex(2, 2) | 1ull << coordinateToIndex(2, 3) |
        // 1ull << coordinateToIndex(2, 4) | 1ull << coordinateToIndex(2, 5) |
        1ull << coordinateToIndex(3, 2);
#else
    // 先手（黒）
    piece[0] = 1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 1) |
               1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 0);
    // 後手（白）
    piece[1] = 1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 1) |
               1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 0);
#endif

    for (int y = 0; y < columns; y++) {
        for (int x = 0; x < rows; x++) {
            int put_bit = coordinateToIndex(x, y);
            uint64_t put_mask = 1ull << put_bit;
            board_mask |= put_mask;
        }
    }

    // multi_thread(thread_test0, 0, concurrency);

    {
        MyTimer myTimer;
        int variation = simuration(piece[0], piece[1]);
    }

    return 0;
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

    for (int i = 0; i < concurrency; i++) {
        threads.emplace_back(std::thread(func, loop_count, lock));
    }

    for (auto &&thread : threads) {
        thread.join();
    }
}

int simuration(ulong piece_player, ulong piece_rybal)
{
    int num = 0;

    for (int y = 0; y < yNum; y++) {
        for (int x = 0; x < xNum; x++) {
            if (isPlaceable(x, y, piece_player, piece_rybal)) {
                // int piece_bit = coordinateToIndex(x, y);
                // ulong piece_mask = 1ull << piece_bit;
                ulong temp_player = piece_player;
                ulong temp_rybal = piece_rybal;
                Place(x, y, temp_player, temp_rybal);
                num += simuration(temp_rybal, temp_player);
            }
        }
    }

    if (num == 0) {
        ulong tmp = piece_player;
        piece_player = piece_rybal;
        piece_rybal = tmp;

        for (int y = 0; y < yNum; y++) {
            for (int x = 0; x < xNum; x++) {
                if (isPlaceable(x, y, piece_player, piece_rybal)) {
                    // int piece_bit = coordinateToIndex(x, y);
                    // ulong piece_mask = 1ull << piece_bit;
                    ulong temp_player = piece_player;
                    ulong temp_rybal = piece_rybal;
                    Place(x, y, temp_player, temp_rybal);
                    num += simuration(temp_rybal, temp_player);
                }
            }
        }
    }

    return num == 0 ? 1 : num;
    // 置けるセルの数を返す
    // return GetBitCount(placeable[(int)player]);
}

bool bitTest(int x, int y, ulong mask)
{
    int piece_bit = coordinateToIndex(x, y);
    ulong piece_mask = 1ull << piece_bit;
    return (mask & piece_mask) == piece_mask;
}

bool IsExist(int x, int y, ulong mask) { return bitTest(x, y, mask); }

bool isPlaceable(int x, int y, ulong piece_player, ulong piece_rybal)
{
    // ePLAYER rybal = player == ePLAYER.P0 ? ePLAYER.P1 : ePLAYER.P0;

    // playerまたはrybalがすでに置いているセル
    if (bitTest(x, y, piece_player) || bitTest(x, y, piece_rybal) ||
        bitTest(x, y, ~board_mask)) {
        return false;
    }

    // 8方向にそれぞれに繰り返す
    // foreach (var v in searchVec) {
    for (int i = 0; i < 8; i++) {
        int step = 0;  // 検索の状態
                       // （0:挟まれる相手の駒を探索中，1:挟む自分の駒を探索中）

        int v_X = searchVec[i][0];
        int v_Y = searchVec[i][1];

        for (int dx = x + v_X, dy = y + v_Y;
             0 <= dx && dx < xNum && 0 <= dy && dy < yNum;
             dx += v_X, dy += v_Y) {
            if (step == 0) {
                if (bitTest(dx, dy, piece_rybal)) {
                    step++;
                    continue;
                } else {
                    break;
                }
            } else if (step == 1) {
                if (bitTest(dx, dy, piece_player)) {
                    return true;
                } else if (bitTest(dx, dy, piece_rybal)) {
                    continue;
                } else {
                    break;
                }
            }
        }
    }

    return false;
}

void Place(int x, int y, ulong &piece_player, ulong &piece_rybal)
{
    int piece_bit = coordinateToIndex(x, y);
    ulong piece_mask = 1ull << piece_bit;

    piece_player |= piece_mask;

    // foreach (var v in searchVec) {
    for (int i = 0; i < 8; i++) {
        int step = 0;

        int v_X = searchVec[i][0];
        int v_Y = searchVec[i][1];

        for (int dx = x + v_X, dy = y + v_Y;
             0 <= dx && dx < xNum && 0 <= dy && dy < yNum;
             dx += v_X, dy += v_Y) {
            if (step == 0) {
                if (IsExist(dx, dy, piece_rybal)) {
                    step++;
                    continue;
                } else {
                    break;
                }
            } else if (step == 1) {
                if (IsExist(dx, dy, piece_player)) {
                    while (0 <= dx && dx < xNum && 0 <= dy && dy < yNum &&
                           !(dx == x && dy == y)) {
                        piece_bit = coordinateToIndex(dx, dy);
                        piece_mask = 1ul << piece_bit;
                        piece_player |= piece_mask;
                        piece_rybal = (piece_rybal & ~piece_mask);

                        dx -= v_X;
                        dy -= v_Y;
                    }
                    break;
                } else if (IsExist(dx, dy, piece_rybal)) {
                    continue;
                } else {
                    break;
                }
            }
        }
    }
}
