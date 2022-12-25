#ifndef BIT_UTIL_H
#define BIT_UTIL_H

uint64_t empty_board = 0;

#ifndef __GNUC__
int GetNumberOfTrailingZeros(uint64_t x) { return std::countr_zero(x); }
#elif true
int number_of_trailing_zeros_table[64];
int GetNumberOfTrailingZeros(int64_t x)
{
    if (x == 0) return 64;

    uint64_t y = (uint64_t)(x & -x);
    int i = (int)((y * 0x03F566ED27179461ull) >> 58);
    return number_of_trailing_zeros_table[i];
}
#else
int GetNumberOfTrailingZeros(uint64_t x) { return __builtin_ctzll(x); }
#endif

void initialize_bit_util(void)
{
#ifndef __GNUC__
#elif true
    uint64_t hash = 0x03F566ED27179461ull;
    for (int i = 0; i < 64; i++) {
        number_of_trailing_zeros_table[hash >> 58] = i;
        hash <<= 1;
    }
#else
#endif

    for (int y = 0; y < columns; y++) {
        for (int x = 0; x < rows; x++) {
            int put_bit = coordinateToIndex(x, y);
            uint64_t put_mask = 1ull << put_bit;
            empty_board |= put_mask;
        }
    }
}

uint64_t vertical_mirror(uint64_t b)
{
    b = ((b >> 8) & 0x00FF00FF00FF00FFULL) | ((b << 8) & 0xFF00FF00FF00FF00ULL);
    b = ((b >> 16) & 0x0000FFFF0000FFFFULL) | ((b << 16) & 0xFFFF0000FFFF0000ULL);
    b = ((b >> 32) & 0x00000000FFFFFFFFULL) | ((b << 32) & 0xFFFFFFFF00000000ULL);
    return b;
}

uint64_t horizontal_mirror(uint64_t b)
{
    b = ((b >> 1) & 0x5555555555555555ULL) | ((b << 1) & 0xAAAAAAAAAAAAAAAAULL);
    b = ((b >> 2) & 0x3333333333333333ULL) | ((b << 2) & 0xCCCCCCCCCCCCCCCCULL);
    b = ((b >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((b << 4) & 0xF0F0F0F0F0F0F0F0ULL);
    return b;
}

uint64_t transpose(uint64_t b)
{
    uint64_t t;
    t = (b ^ (b >> 7)) & 0x00AA00AA00AA00AAULL;
    b = b ^ t ^ (t << 7);
    t = (b ^ (b >> 14)) & 0x0000CCCC0000CCCCULL;
    b = b ^ t ^ (t << 14);
    t = (b ^ (b >> 28)) & 0x00000000F0F0F0F0ULL;
    b = b ^ t ^ (t << 28);
    return b;
}

uint64_t symmetry_naive(const uint32_t s, uint64_t b)
{
    uint64_t answer = b;
    if (s & 1) {
        answer = horizontal_mirror(answer);
        answer = answer >> (8 - rows);
    }
    if (s & 2) {
        answer = vertical_mirror(answer);
        answer = answer >> (8 * (8 - columns));
    }
    if (s & 4) {
        answer = transpose(answer);
    }
    return answer;
}

void board_symmetry(const uint64_t board[], std::pair<uint64_t, uint64_t> b[8])
{
    for (uint32_t i = 0; i < 8; i++) {
        b[i] = { symmetry_naive(i, board[0]), symmetry_naive(i, board[1]) };
    }
}

uint64_t transfer(const uint64_t put, const int k)
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

uint64_t makeLegalBoard(const uint64_t board[], const int player)
{
    const int opponent = player ^ 1;

    // 左右端の番人
    uint64_t horizontalWatchBoard = board[opponent] & 0x7e7e7e7e7e7e7e7e;
    // 上下端の番人
    uint64_t verticalWatchBoard = board[opponent] & 0x00FFFFFFFFFFFF00;
    // 全辺の番人
    uint64_t allSideWatchBoard = board[opponent] & 0x007e7e7e7e7e7e00;
    // 空きマスのみにビットが立っているボード
    uint64_t blankBoard = empty_board & ~(board[player] | board[opponent]);
    // 隣に相手の色があるかを一時保存する
    uint64_t tmp;
    // 返り値
    uint64_t legalBoard;

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
#endif  // BIT_UTIL_H
