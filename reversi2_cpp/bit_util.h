#ifndef BIT_UTIL_H
#define BIT_UTIL_H

uint64_t empty_board = 0;

int symmetry_table[8][64] = {};

uint64_t symmetry_naive(const uint32_t s, uint64_t b);

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

    for (int dir = 0; dir < 8; dir++) {
        for (int bit = 0; bit < 64; bit++) {
            symmetry_table[dir][bit] = std::countr_zero(symmetry_naive(dir, 1ull << bit));
        }
    }
}

uint64_t vertical_mirror(uint64_t b)
{
#if false
    b = ((b >> 8) & 0x00FF00FF00FF00FFULL) | ((b << 8) & 0xFF00FF00FF00FF00ULL);
    b = ((b >> 16) & 0x0000FFFF0000FFFFULL) | ((b << 16) & 0xFFFF0000FFFF0000ULL);
    b = ((b >> 32) & 0x00000000FFFFFFFFULL) | ((b << 32) & 0xFFFFFFFF00000000ULL);
    return b;
#else
    return _byteswap_uint64(b);
#endif
}

uint64_t horizontal_mirror(uint64_t b)
{
#if false
    b = ((b >> 1) & 0x5555555555555555ULL) | ((b << 1) & 0xAAAAAAAAAAAAAAAAULL);
    b = ((b >> 2) & 0x3333333333333333ULL) | ((b << 2) & 0xCCCCCCCCCCCCCCCCULL);
    b = ((b >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((b << 4) & 0xF0F0F0F0F0F0F0F0ULL);
#else
    b = ((b >> 1) & 0x5555555555555555ULL) | ((b & 0x5555555555555555ULL) << 1);
    b = ((b >> 2) & 0x3333333333333333ULL) | ((b & 0x3333333333333333ULL) << 2);
    b = ((b >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((b & 0x0F0F0F0F0F0F0F0FULL) << 4);
#endif
    return b;
}

uint64_t transpose_bitboard_avx2(uint64_t b)
{
    //引数が8x8 bitboardだとして、転置して返す。

    const __m256i bb = _mm256_set1_epi64x(b);
    const __m256i x1 = _mm256_sllv_epi64(bb, _mm256_set_epi64x(0, 1, 2, 3));
    const __m256i x2 = _mm256_sllv_epi64(bb, _mm256_set_epi64x(4, 5, 6, 7));
    const int y1 = _mm256_movemask_epi8(x1);
    const int y2 = _mm256_movemask_epi8(x2);

    return (uint64_t(uint32_t(y1)) << 32) + uint64_t(uint32_t(y2));
}

uint64_t transpose(uint64_t b)
{
#if true
    uint64_t t;
    t = (b ^ (b >> 7)) & 0x00AA00AA00AA00AAULL;
    b = b ^ t ^ (t << 7);
    t = (b ^ (b >> 14)) & 0x0000CCCC0000CCCCULL;
    b = b ^ t ^ (t << 14);
    t = (b ^ (b >> 28)) & 0x00000000F0F0F0F0ULL;
    b = b ^ t ^ (t << 28);
    return b;
#else
    return transpose_bitboard_avx2(b);
#endif
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

//https://primenumber.hatenadiary.jp/entry/2016/12/26/063226
//http://www.amy.hi-ho.ne.jp/okuhara/flipcuda.htm
struct u64_4 {
    __m256i data;
    u64_4() = default;
    u64_4(uint64_t val)
        : data(_mm256_set1_epi64x(val))
    {
    }
    u64_4(uint64_t x, uint64_t y, uint64_t z, uint64_t w)
        : data(_mm256_set_epi64x(x, y, z, w))
    {
    }
    u64_4(__m256i data) : data(data) {}
    operator __m256i() { return data; }
};


inline u64_4 operator>>(const u64_4 lhs, const size_t n)
{
    return _mm256_srli_epi64(lhs.data, n);
}

inline u64_4 operator<<(const u64_4 lhs, const size_t n)
{
    return _mm256_slli_epi64(lhs.data, n);
}

inline u64_4 operator&(const u64_4 lhs, const u64_4 rhs)
{
    return _mm256_and_si256(lhs.data, rhs.data);
}

inline u64_4 operator|(const u64_4 lhs, const u64_4 rhs)
{
    return _mm256_or_si256(lhs.data, rhs.data);
}

inline u64_4 operator+(const u64_4 lhs, const u64_4 rhs)
{
    return _mm256_add_epi64(lhs.data, rhs.data);
}

inline u64_4 operator+(const u64_4 lhs, const uint64_t rhs)
{
    __m256i r64 = _mm256_set1_epi64x(rhs);
    return _mm256_add_epi64(lhs.data, r64);
}

inline u64_4 operator-(const u64_4 lhs, const u64_4 rhs)
{
    return _mm256_sub_epi64(lhs.data, rhs.data);
}

inline u64_4 operator-(const u64_4 lhs)
{
    return _mm256_sub_epi64(_mm256_setzero_si256(), lhs.data);
}

// (NOT lhs) AND rhs
inline u64_4 andnot(const u64_4 lhs, const u64_4 rhs)
{
    return _mm256_andnot_si256(lhs.data, rhs.data);
}

inline u64_4 operator~(const u64_4 lhs)
{
    return _mm256_andnot_si256(lhs.data, _mm256_set1_epi8(0xFF));
}

inline u64_4 nonzero(const u64_4 lhs)
{
    return _mm256_cmpeq_epi64(lhs.data, _mm256_setzero_si256()) + u64_4(1);
}

inline __m128i hor(const u64_4 lhs)
{
    __m128i lhs_xz_yw = _mm_or_si128(_mm256_castsi256_si128(lhs.data),
        _mm256_extractf128_si256(lhs.data, 1));
    return _mm_or_si128(lhs_xz_yw, _mm_alignr_epi8(lhs_xz_yw, lhs_xz_yw, 8));
}

alignas(32) __m256i flip_vertical_shuffle_table_256 = _mm256_set_epi8(
    24, 25, 26, 27, 28, 29, 30, 31,
    16, 17, 18, 19, 20, 21, 22, 23,
    8, 9, 10, 11, 12, 13, 14, 15,
    0, 1, 2, 3, 4, 5, 6, 7);

__m256i flipVertical(__m256i dbd)
{
    return _mm256_shuffle_epi8(dbd, flip_vertical_shuffle_table_256);
}

inline u64_4 upper_bit(u64_4 p)
{
    p = p | (p >> 1);
    p = p | (p >> 2);
    p = p | (p >> 4);
    p = andnot(p >> 1, p);
    p = (__m256i)flipVertical(__m256i(p));
    p = p & -p;
    return (__m256i)flipVertical(__m256i(p));
}

inline __m256i broadcast_black(__m128i bd)
{
    return _mm256_broadcastq_epi64(bd);
}

inline __m256i broadcast_white(__m128i bd)
{
    return _mm256_permute4x64_epi64(_mm256_castsi128_si256(bd), 0x55);
}

//_mm_setr_epi64x(black, white)

__m128i flip(__m128i bd, int pos)
{
    u64_4 black = broadcast_black(bd);
    u64_4 white = broadcast_white(bd);
    u64_4 flipped, OM, outflank, mask;
    u64_4 yzw = {
      UINT64_C(0xFFFFFFFFFFFFFFFF),
      UINT64_C(0x7E7E7E7E7E7E7E7E),
      UINT64_C(0x7E7E7E7E7E7E7E7E),
      UINT64_C(0x7E7E7E7E7E7E7E7E)
    };
    OM = white & yzw;
    mask = {
      UINT64_C(0x0080808080808080),
      UINT64_C(0x7F00000000000000),
      UINT64_C(0x0102040810204000),
      UINT64_C(0x0040201008040201)
    };
    mask = mask >> (63 - pos);
    outflank = upper_bit(andnot(OM, mask)) & black;
    flipped = (-outflank << 1) & mask;
    mask = {
      UINT64_C(0x0101010101010100),
      UINT64_C(0x00000000000000FE),
      UINT64_C(0x0002040810204080),
      UINT64_C(0x8040201008040200)
    };
    mask = mask << pos;
    outflank = mask & ((OM | ~mask) + 1) & black;
    flipped = flipped | ((outflank - nonzero(outflank)) & mask);
    return hor(flipped);
}

#if false
https://github.com/primenumber/issen/blob/master/src/bit_manipulations.cpp
uint64_t delta_swap(uint64_t bits, uint64_t mask, int delta)
{
    uint64_t tmp = mask & (bits ^ (bits << delta));
    return bits ^ tmp ^ (tmp >> delta);
}

uint64_t flipDiagA1H8(uint64_t bits)
{
    uint64_t mask1 = UINT64_C(0x5500550055005500);
    uint64_t mask2 = UINT64_C(0x3333000033330000);
    uint64_t mask3 = UINT64_C(0x0f0f0f0f00000000);
    bits = delta_swap(bits, mask3, 28);
    bits = delta_swap(bits, mask2, 14);
    return delta_swap(bits, mask1, 7);
}

uint64_t flipDiagA8H1(uint64_t bits)
{
    uint64_t mask1 = UINT64_C(0xaa00aa00aa00aa00);
    uint64_t mask2 = UINT64_C(0xcccc0000cccc0000);
    uint64_t mask3 = UINT64_C(0xf0f0f0f000000000);
    bits = delta_swap(bits, mask3, 36);
    bits = delta_swap(bits, mask2, 18);
    return delta_swap(bits, mask1, 9);
}

uint64_t rotate90clockwise(uint64_t bits)
{
    return vertical_mirror(flipDiagA8H1(bits));
}

uint64_t rotate90antiClockwise(uint64_t bits)
{
    return vertical_mirror(flipDiagA1H8(bits));
}

uint64_t rotr(uint64_t bits, int index)
{
    return _rotr64(bits, index);
}

uint64_t pseudoRotate45clockwise(uint64_t bits)
{
    uint64_t mask1 = UINT64_C(0x5555555555555555);
    uint64_t mask2 = UINT64_C(0x3333333333333333);
    uint64_t mask3 = UINT64_C(0x0f0f0f0f0f0f0f0f);
    bits = bits ^ (mask1 & (bits ^ rotr(bits, 8)));
    bits = bits ^ (mask2 & (bits ^ rotr(bits, 16)));
    return bits ^ (mask3 & (bits ^ rotr(bits, 32)));
}

uint64_t pseudoRotate45antiClockwise(uint64_t bits)
{
    uint64_t mask1 = UINT64_C(0xaaaaaaaaaaaaaaaa);
    uint64_t mask2 = UINT64_C(0xcccccccccccccccc);
    uint64_t mask3 = UINT64_C(0xf0f0f0f0f0f0f0f0);
    bits = bits ^ (mask1 & (bits ^ rotr(bits, 8)));
    bits = bits ^ (mask2 & (bits ^ rotr(bits, 16)));
    return bits ^ (mask3 & (bits ^ rotr(bits, 32)));
}
#endif

//https://primenumber.hatenadiary.jp/entry/2016/12/26/063226
//uint8_t mobility_line(uint8_t p, uint8_t o)
//{
//    uint8_t p1 = p << 1;
//    return ~(p1 | o) & (p1 + o);
//}

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
#if false   // todo:uint64_tを__m256iに書き換える
    {
        __m128i p = _mm_set_epi64x(board[player], horizontal_mirror(board[player]));    // p 左右
        __m128i p1 = _mm_add_epi8(p, p);                                                // p << 1
        __m128i o = _mm_set_epi64x(board[opponent], horizontal_mirror(board[opponent]));// o 左右
        __m128i rt = _mm_andnot_si128(_mm_or_si128(p1, o), _mm_add_epi64(p1, o));       // ~(p1 | o) & (p1 + o)
        legalBoard = rt.m128i_u64[1] | horizontal_mirror(rt.m128i_u64[0]);
    }
#endif
    // 左
    tmp = horizontalWatchBoard & (board[player] << 1);
#if ROWS >= 8
    tmp |= horizontalWatchBoard & (tmp << 1);
#endif
#if ROWS >= 7
    tmp |= horizontalWatchBoard & (tmp << 1);
#endif
    tmp |= horizontalWatchBoard & (tmp << 1);
    tmp |= horizontalWatchBoard & (tmp << 1);
    tmp |= horizontalWatchBoard & (tmp << 1);
    legalBoard = (tmp << 1);

    // 右
    tmp = horizontalWatchBoard & (board[player] >> 1);
#if ROWS >= 8
    tmp |= horizontalWatchBoard & (tmp >> 1);
#endif
#if ROWS >= 7
    tmp |= horizontalWatchBoard & (tmp >> 1);
#endif
    tmp |= horizontalWatchBoard & (tmp >> 1);
    tmp |= horizontalWatchBoard & (tmp >> 1);
    tmp |= horizontalWatchBoard & (tmp >> 1);
    legalBoard |= (tmp >> 1);

    // 上
    tmp = verticalWatchBoard & (board[player] << 8);
#if COLUMNS >= 8
    tmp |= verticalWatchBoard & (tmp << 8);
#endif
#if COLUMNS >= 7
    tmp |= verticalWatchBoard & (tmp << 8);
#endif
    tmp |= verticalWatchBoard & (tmp << 8);
    tmp |= verticalWatchBoard & (tmp << 8);
    tmp |= verticalWatchBoard & (tmp << 8);
    legalBoard |= (tmp << 8);

    // 下
    tmp = verticalWatchBoard & (board[player] >> 8);
#if COLUMNS >= 8
    tmp |= verticalWatchBoard & (tmp >> 8);
#endif
#if COLUMNS >= 7
    tmp |= verticalWatchBoard & (tmp >> 8);
#endif
    tmp |= verticalWatchBoard & (tmp >> 8);
    tmp |= verticalWatchBoard & (tmp >> 8);
    tmp |= verticalWatchBoard & (tmp >> 8);
    legalBoard |= (tmp >> 8);

    // 右斜め上
    tmp = allSideWatchBoard & (board[player] << 7);
#if ROWS >= 8 && COLUMNS >= 8
    tmp |= allSideWatchBoard & (tmp << 7);
#endif
#if ROWS >= 7 && COLUMNS >= 7
    tmp |= allSideWatchBoard & (tmp << 7);
#endif
    tmp |= allSideWatchBoard & (tmp << 7);
    tmp |= allSideWatchBoard & (tmp << 7);
    tmp |= allSideWatchBoard & (tmp << 7);
    legalBoard |= (tmp << 7);

    // 左斜め上
    tmp = allSideWatchBoard & (board[player] << 9);
#if ROWS >= 8 && COLUMNS >= 8
    tmp |= allSideWatchBoard & (tmp << 9);
#endif
#if ROWS >= 7 && COLUMNS >= 7
    tmp |= allSideWatchBoard & (tmp << 9);
#endif
    tmp |= allSideWatchBoard & (tmp << 9);
    tmp |= allSideWatchBoard & (tmp << 9);
    tmp |= allSideWatchBoard & (tmp << 9);
    legalBoard |= (tmp << 9);

    // 右斜め下
    tmp = allSideWatchBoard & (board[player] >> 9);
#if ROWS >= 8 && COLUMNS >= 8
    tmp |= allSideWatchBoard & (tmp >> 9);
#endif
#if ROWS >= 7 && COLUMNS >= 7
    tmp |= allSideWatchBoard & (tmp >> 9);
#endif
    tmp |= allSideWatchBoard & (tmp >> 9);
    tmp |= allSideWatchBoard & (tmp >> 9);
    tmp |= allSideWatchBoard & (tmp >> 9);
    legalBoard |= (tmp >> 9);

    // 左斜め下
    tmp = allSideWatchBoard & (board[player] >> 7);
#if ROWS >= 8 && COLUMNS >= 8
    tmp |= allSideWatchBoard & (tmp >> 7);
#endif
#if ROWS >= 7 && COLUMNS >= 7
    tmp |= allSideWatchBoard & (tmp >> 7);
#endif
    tmp |= allSideWatchBoard & (tmp >> 7);
    tmp |= allSideWatchBoard & (tmp >> 7);
    tmp |= allSideWatchBoard & (tmp >> 7);
    legalBoard |= (tmp >> 7);

    return legalBoard & blankBoard;
}

uint64_t moveOrderingTable[5] = {
    1ull << coordinateToIndex(0, 0) |
    1ull << coordinateToIndex(5, 0) |
    1ull << coordinateToIndex(0, 5) |
    1ull << coordinateToIndex(5, 5),

    1ull << coordinateToIndex(2, 0) |
    1ull << coordinateToIndex(3, 0) |
    1ull << coordinateToIndex(0, 2) |
    1ull << coordinateToIndex(5, 2) |
    1ull << coordinateToIndex(0, 3) |
    1ull << coordinateToIndex(5, 3) |
    1ull << coordinateToIndex(2, 5) |
    1ull << coordinateToIndex(3, 5),

    1ull << coordinateToIndex(2, 1) |
    1ull << coordinateToIndex(3, 1) |
    1ull << coordinateToIndex(1, 2) |
    1ull << coordinateToIndex(4, 2) |
    1ull << coordinateToIndex(1, 3) |
    1ull << coordinateToIndex(4, 3) |
    1ull << coordinateToIndex(2, 4) |
    1ull << coordinateToIndex(3, 4),

    1ull << coordinateToIndex(1, 0) |
    1ull << coordinateToIndex(4, 0) |
    1ull << coordinateToIndex(0, 1) |
    1ull << coordinateToIndex(5, 1) |
    1ull << coordinateToIndex(0, 4) |
    1ull << coordinateToIndex(5, 4) |
    1ull << coordinateToIndex(1, 5) |
    1ull << coordinateToIndex(4, 5),

    1ull << coordinateToIndex(1, 1) |
    1ull << coordinateToIndex(4, 1) |
    1ull << coordinateToIndex(1, 4) |
    1ull << coordinateToIndex(4, 4),
};

//uint64_t moveOrdering[20] = {
//    0, 25, 35, 2, 24, 20, 11, 17, 29, 43, 16, 13, 28, 3, 42, 34, 1, 0, 4, 37,
//};

#ifndef __GNUC__
int GetNumberOfTrailingZeros(uint64_t x, const int hierarchy)
{
    //if (hierarchy < sizeof(moveOrdering) / sizeof(moveOrdering[0]))
    //{
    //    if (x & (1ull << moveOrdering[hierarchy])) {
    //        return moveOrdering[hierarchy];
    //    }
    //}

#if OPT_MOVE_ORDERING_6x6
    int bit;
    if ((bit = std::countr_zero(x & moveOrderingTable[0])) != 64) { return bit; }
    if ((bit = std::countr_zero(x & moveOrderingTable[1])) != 64) { return bit; }
    if ((bit = std::countr_zero(x & moveOrderingTable[2])) != 64) { return bit; }
    if ((bit = std::countr_zero(x & moveOrderingTable[3])) != 64) { return bit; }
    if ((bit = std::countr_zero(x & moveOrderingTable[4])) != 64) { return bit; }
    return std::countr_zero(x);
#else
    return std::countr_zero(x);
#endif
}
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

#endif  // BIT_UTIL_H
