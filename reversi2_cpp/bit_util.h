#ifndef BIT_UTIL_H
#define BIT_UTIL_H

alignas(32) __m128i rotr8_shuffle_table[8];
alignas(32) __m256i rotr8_shuffle_table_256[8];

struct half_board {
    uint64_t data;
    half_board() = default;
    constexpr half_board(const half_board&) = default;
    constexpr half_board(half_board&&) = default;
    constexpr half_board(const uint64_t data) : data(data) {}
    half_board& operator=(const half_board&) = default;
    half_board& operator=(half_board&&) = default;
    operator uint64_t() { return data; }
    operator uint64_t() const { return data; }
    bool get(const size_t index) const
    {
        return _bittest64((__int64*)&data, index);
    }
    bool set(const size_t index)
    {
        return _bittestandset64((__int64*)&data, index);
    }
    bool reset(const size_t index)
    {
        return _bittestandreset64((__int64*)&data, index);
    }
    bool setval(const size_t index, const bool bit)
    {
        return bit ? set(index) : reset(index);
    }
};

struct board {
    __m128i data;
    board() = default;
    board(const board&) = default;
    board(const uint64_t black, const uint64_t white)
        : data(_mm_setr_epi64x(black, white))
    {
    }
    board(__m128i data) : data(data) {}
    operator __m128i() { return data; }
    operator __m128i() const { return data; }
    board& operator=(const board&) = default;
    board& operator=(board&&) = default;
    const half_board black() const { return _mm_cvtsi128_si64(data); }
    const half_board white() const { return _mm_extract_epi64(data, 1); }
    static board initial_board()
    {
        return board(UINT64_C(0x0000000810000000),
            UINT64_C(0x0000001008000000));
    }
    static board empty_board() { return board(_mm_setzero_si128()); }
    static board reverse_board(const board& bd)
    {
        return board(_mm_alignr_epi8(bd.data, bd.data, 8));
    }
};

struct double_board {
    __m256i data;
    double_board() = default;
    double_board(const double_board&) = default;
    double_board(const board& bd1, const board& bd2)
        : data(_mm256_setr_m128i(bd1, bd2))
    {
    }
    explicit double_board(const board& bd)
        : data(_mm256_broadcastsi128_si256(bd))
    {
    }
    double_board(const uint64_t black1, const uint64_t white1,
        const uint64_t black2, const uint64_t white2)
        : data(_mm256_setr_epi64x(black1, white1, black2, white2))
    {
    }
    double_board(const __m256i data) : data(data) {}
    operator __m256i() { return data; }
    operator __m256i() const { return data; }
    double_board& operator=(const double_board&) = default;
    double_board& operator=(double_board&&) = default;
    const board board1() const { return _mm256_castsi256_si128(data); }
    const board board2() const { return _mm256_extracti128_si256(data, 1); }
    uint64_t operator[](const size_t index)
    {
        switch (index) {
        case 0:
            return _mm256_extract_epi64(data, 0);
        case 1:
            return _mm256_extract_epi64(data, 1);
        case 2:
            return _mm256_extract_epi64(data, 2);
        case 3:
            return _mm256_extract_epi64(data, 3);
        default:
            return 0;
        }
    }
};

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
            symmetry_table[dir][bit] =
                std::countr_zero(symmetry_naive(dir, 1ull << bit));
        }
    }
    for (int i = 0; i < 8; ++i) {
        rotr8_shuffle_table[i] = _mm_set_epi8(
            (7 + i) % 8 + 8, (6 + i) % 8 + 8, (5 + i) % 8 + 8, (4 + i) % 8 + 8,
            (3 + i) % 8 + 8, (2 + i) % 8 + 8, (1 + i) % 8 + 8, (0 + i) % 8 + 8,
            (7 + i) % 8, (6 + i) % 8, (5 + i) % 8, (4 + i) % 8, (3 + i) % 8,
            (2 + i) % 8, (1 + i) % 8, (0 + i) % 8);
        rotr8_shuffle_table_256[i] = _mm256_set_epi8(
            (7 + i) % 8 + 24, (6 + i) % 8 + 24, (5 + i) % 8 + 24,
            (4 + i) % 8 + 24, (3 + i) % 8 + 24, (2 + i) % 8 + 24,
            (1 + i) % 8 + 24, (0 + i) % 8 + 24, (7 + i) % 8 + 16,
            (6 + i) % 8 + 16, (5 + i) % 8 + 16, (4 + i) % 8 + 16,
            (3 + i) % 8 + 16, (2 + i) % 8 + 16, (1 + i) % 8 + 16,
            (0 + i) % 8 + 16, (7 + i) % 8 + 8, (6 + i) % 8 + 8, (5 + i) % 8 + 8,
            (4 + i) % 8 + 8, (3 + i) % 8 + 8, (2 + i) % 8 + 8, (1 + i) % 8 + 8,
            (0 + i) % 8 + 8, (7 + i) % 8, (6 + i) % 8, (5 + i) % 8, (4 + i) % 8,
            (3 + i) % 8, (2 + i) % 8, (1 + i) % 8, (0 + i) % 8);
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
    // 引数が8x8 bitboardだとして、転置して返す。

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
        // answer = answer >> (8 - rows);
    }
    if (s & 2) {
        answer = vertical_mirror(answer);
        // answer = answer >> (8 * (8 - columns));
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

// https://primenumber.hatenadiary.jp/entry/2016/12/26/063226
// http://www.amy.hi-ho.ne.jp/okuhara/flipcuda.htm
struct u64_4 {
    __m256i data;
    u64_4() = default;
    u64_4(uint64_t val) : data(_mm256_set1_epi64x(val)) {}
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

alignas(32) __m256i flip_vertical_shuffle_table_256 =
_mm256_set_epi8(24, 25, 26, 27, 28, 29, 30, 31, 16, 17, 18, 19, 20, 21, 22,
    23, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7);

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
    u64_4 yzw = { UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0x7E7E7E7E7E7E7E7E),
                 UINT64_C(0x7E7E7E7E7E7E7E7E), UINT64_C(0x7E7E7E7E7E7E7E7E) };
    OM = white & yzw;
    mask = { UINT64_C(0x0080808080808080), UINT64_C(0x7F00000000000000),
            UINT64_C(0x0102040810204000), UINT64_C(0x0040201008040201) };
    mask = mask >> (63 - pos);
    outflank = upper_bit(andnot(OM, mask)) & black;
    flipped = (-outflank << 1) & mask;
    mask = { UINT64_C(0x0101010101010100), UINT64_C(0x00000000000000FE),
            UINT64_C(0x0002040810204080), UINT64_C(0x8040201008040200) };
    mask = mask << pos;
    outflank = mask & ((OM | ~mask) + 1) & black;
    flipped = flipped | ((outflank - nonzero(outflank)) & mask);
    return hor(flipped);
}

#if true
// https://github.com/primenumber/issen/blob/master/src/bit_manipulations.cpp
// https://qiita.com/ysuzuk81/items/453b08a14d23fb8c6c11
uint64_t delta_swap(uint64_t bits, uint64_t mask, int delta)
{
    uint64_t tmp = mask & (bits ^ (bits << delta));
    return bits ^ tmp ^ (tmp >> delta);
}

__m128i delta_swap(__m128i bits, __m128i mask, int delta)
{
    __m128i tmp =
        _mm_and_si128(mask, _mm_xor_si128(bits, _mm_slli_epi64(bits, delta)));
    return _mm_xor_si128(_mm_xor_si128(bits, tmp), _mm_srli_epi64(tmp, delta));
}

__m256i delta_swap(__m256i bits, __m256i mask, int delta)
{
    __m256i tmp = _mm256_and_si256(
        mask, _mm256_xor_si256(bits, _mm256_slli_epi64(bits, delta)));
    return _mm256_xor_si256(_mm256_xor_si256(bits, tmp),
        _mm256_srli_epi64(tmp, delta));
}

double_board flipDiagA1H8(double_board dbd)
{
    __m256i mask1 = _mm256_set1_epi16(0x5500);
    __m256i mask2 = _mm256_set1_epi32(0x33330000);
    __m256i mask3 = _mm256_set1_epi64x(UINT64_C(0x0f0f0f0f00000000));
    __m256i data = delta_swap(dbd, mask3, 28);
    data = delta_swap(data, mask2, 14);
    return double_board(delta_swap(data, mask1, 7));
}

board flipDiagA1H8(board bd)
{
    __m128i mask1 = _mm_set1_epi16(0x5500);
    __m128i mask2 = _mm_set1_epi32(0x33330000);
    __m128i mask3 = _mm_set1_epi64x(UINT64_C(0x0f0f0f0f00000000));
    __m128i data = delta_swap(bd, mask3, 28);
    data = delta_swap(data, mask2, 14);
    return board(delta_swap(data, mask1, 7));
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

__m256i rotr8_epi64(__m256i bits, int index)
{
    return _mm256_shuffle_epi8(bits, rotr8_shuffle_table_256[index]);
}

__m128i rotr8_epi64(__m128i bits, int index)
{
    return _mm_shuffle_epi8(bits, rotr8_shuffle_table[index]);
}

uint64_t rotr(uint64_t bits, int index) { return _rotr64(bits, index); }

double_board pseudoRotate45clockwise(double_board dbd)
{
    __m256i mask1 = _mm256_set1_epi8(0x55);
    __m256i mask2 = _mm256_set1_epi8(0x33);
    __m256i mask3 = _mm256_set1_epi8(0x0f);
    __m256i data = _mm256_xor_si256(
        __m256i(dbd),
        _mm256_and_si256(mask1,
            _mm256_xor_si256(__m256i(dbd), rotr8_epi64(dbd, 1))));
    data = _mm256_xor_si256(
        data,
        _mm256_and_si256(mask2, _mm256_xor_si256(data, rotr8_epi64(data, 2))));
    return _mm256_xor_si256(
        data,
        _mm256_and_si256(mask3, _mm256_xor_si256(data, rotr8_epi64(data, 4))));
}

board pseudoRotate45clockwise(board bd)
{
    __m128i mask1 = _mm_set1_epi8(0x55);
    __m128i mask2 = _mm_set1_epi8(0x33);
    __m128i mask3 = _mm_set1_epi8(0x0f);
    __m128i data = _mm_xor_si128(
        __m128i(bd),
        _mm_and_si128(mask1, _mm_xor_si128(__m128i(bd), rotr8_epi64(bd, 1))));
    data = _mm_xor_si128(
        data, _mm_and_si128(mask2, _mm_xor_si128(data, rotr8_epi64(data, 2))));
    return _mm_xor_si128(
        data, _mm_and_si128(mask3, _mm_xor_si128(data, rotr8_epi64(data, 4))));
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

double_board pseudoRotate45antiClockwise(double_board dbd)
{
    __m256i mask1 = _mm256_set1_epi8(0xaa);
    __m256i mask2 = _mm256_set1_epi8(0xcc);
    __m256i mask3 = _mm256_set1_epi8(0xf0);
    __m256i data = _mm256_xor_si256(
        __m256i(dbd),
        _mm256_and_si256(mask1,
            _mm256_xor_si256(__m256i(dbd), rotr8_epi64(dbd, 1))));
    data = _mm256_xor_si256(
        data,
        _mm256_and_si256(mask2, _mm256_xor_si256(data, rotr8_epi64(data, 2))));
    return _mm256_xor_si256(
        data,
        _mm256_and_si256(mask3, _mm256_xor_si256(data, rotr8_epi64(data, 4))));
}

board pseudoRotate45antiClockwise(board bd)
{
    __m128i mask1 = _mm_set1_epi8(0xaa);
    __m128i mask2 = _mm_set1_epi8(0xcc);
    __m128i mask3 = _mm_set1_epi8(0xf0);
    __m128i data = _mm_xor_si128(
        __m128i(bd),
        _mm_and_si128(mask1, _mm_xor_si128(__m128i(bd), rotr8_epi64(bd, 1))));
    data = _mm_xor_si128(
        data, _mm_and_si128(mask2, _mm_xor_si128(data, rotr8_epi64(data, 2))));
    return _mm_xor_si128(
        data, _mm_and_si128(mask3, _mm_xor_si128(data, rotr8_epi64(data, 4))));
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

// https://primenumber.hatenadiary.jp/entry/2016/12/26/063226
// uint8_t mobility_line(uint8_t p, uint8_t o)
//{
//     uint8_t p1 = p << 1;
//     return ~(p1 | o) & (p1 + o);
// }

double_board mirrorHorizontal(double_board dbd)
{
    __m256i mask1 = _mm256_set1_epi8(0x55);
    __m256i mask2 = _mm256_set1_epi8(0x33);
    __m256i mask3 = _mm256_set1_epi8(0x0f);
    dbd = _mm256_or_si256(_mm256_and_si256(_mm256_srli_epi64(dbd, 1), mask1),
        _mm256_slli_epi64(_mm256_and_si256(dbd, mask1), 1));
    dbd = _mm256_or_si256(_mm256_and_si256(_mm256_srli_epi64(dbd, 2), mask2),
        _mm256_slli_epi64(_mm256_and_si256(dbd, mask2), 2));
    dbd = _mm256_or_si256(_mm256_and_si256(_mm256_srli_epi64(dbd, 4), mask3),
        _mm256_slli_epi64(_mm256_and_si256(dbd, mask3), 4));
    return dbd;
}

board mirrorHorizontal(board bd)
{
    __m128i mask1 = _mm_set1_epi8(0x55);
    __m128i mask2 = _mm_set1_epi8(0x33);
    __m128i mask3 = _mm_set1_epi8(0x0f);
    bd = _mm_or_si128(_mm_and_si128(_mm_srli_epi64(bd, 1), mask1),
        _mm_slli_epi64(_mm_and_si128(bd, mask1), 1));
    bd = _mm_or_si128(_mm_and_si128(_mm_srli_epi64(bd, 2), mask2),
        _mm_slli_epi64(_mm_and_si128(bd, mask2), 2));
    bd = _mm_or_si128(_mm_and_si128(_mm_srli_epi64(bd, 4), mask3),
        _mm_slli_epi64(_mm_and_si128(bd, mask3), 4));
    return bd;
}

uint64_t mirrorHorizontal(uint64_t bits)
{
    uint64_t mask1 = UINT64_C(0x5555555555555555);
    uint64_t mask2 = UINT64_C(0x3333333333333333);
    uint64_t mask3 = UINT64_C(0x0f0f0f0f0f0f0f0f);
    bits = ((bits >> 1) & mask1) | ((bits & mask1) << 1);
    bits = ((bits >> 2) & mask2) | ((bits & mask2) << 2);
    bits = ((bits >> 4) & mask3) | ((bits & mask3) << 4);
    return bits;
}

board puttable_black_backward_p2(board bd1, board bd2)
{
    __m128i b = _mm_unpacklo_epi64(bd1, bd2);
    __m128i b1 = _mm_add_epi8(b, b);
    __m128i w = _mm_unpackhi_epi64(bd1, bd2);
    return _mm_andnot_si128(_mm_or_si128(b1, w), _mm_add_epi8(b1, w));
}

uint64_t puttable_black_horizontal(const board& bd)
{
    board tmp = puttable_black_backward_p2(bd, mirrorHorizontal(bd));
    return tmp.black() | mirrorHorizontal(tmp.white());
}

double_board mobility_backward_p4(double_board dbd1, double_board dbd2)
{
    __m256i b = _mm256_unpacklo_epi64(dbd1, dbd2);
    __m256i b1 = _mm256_add_epi8(b, b);
    __m256i w = _mm256_unpackhi_epi64(dbd1, dbd2);
    return _mm256_andnot_si256(_mm256_or_si256(b1, w), _mm256_add_epi8(b1, w));
}

uint64_t puttable_black_horizontal_and_vertical(const board& bd)
{
    board bd_h = flipDiagA1H8(bd);
    double_board dbd1(bd, bd_h);
    double_board dbd2 = mirrorHorizontal(dbd1);
    double_board res = mobility_backward_p4(dbd1, dbd2);
    double_board resp = _mm256_permute4x64_epi64(res, 0b11011000);
    board res1 = resp.board1();
    board res2 = mirrorHorizontal(resp.board2());
    board resb = _mm_or_si128(res1, res2);
    return resb.black() | flipDiagA1H8(resb.white());
}

uint64_t puttable_black_diag_implA8H1(const board& bd)
{
    const board prot45_bd = pseudoRotate45clockwise(bd);
    uint64_t mask64 = UINT64_C(0x80C0E0F0F8FCFEFF);
    __m128i mask = _mm_set1_epi64x(mask64);
    uint64_t res =
        (mask64 & puttable_black_horizontal(_mm_and_si128(mask, prot45_bd))) |
        (~mask64 &
            puttable_black_horizontal(_mm_andnot_si128(mask, prot45_bd)));
    return pseudoRotate45antiClockwise(res);
}

uint64_t puttable_black_diag_implA1H8(const board& bd)
{
    const board prot45a_bd = pseudoRotate45antiClockwise(bd);
    uint64_t mask64 = UINT64_C(0xFEFCF8F0E0C08000);
    __m128i mask = _mm_set1_epi64x(mask64);
    uint64_t res =
        (mask64 & puttable_black_horizontal(_mm_and_si128(mask, prot45a_bd))) |
        (~mask64 &
            puttable_black_horizontal(_mm_andnot_si128(mask, prot45a_bd)));
    return pseudoRotate45clockwise(res);
}

uint64_t puttable_black_diag(const board& bd)
{
    return rotr(
        puttable_black_diag_implA8H1(bd) | puttable_black_diag_implA1H8(bd), 8);
}

uint64_t stones(board bd)
{
    return _mm_cvtsi128_si64(_mm_or_si128(_mm_alignr_epi8(bd, bd, 8), bd));
}

uint64_t puttable_black(const board& bd)
{
    return (puttable_black_horizontal_and_vertical(bd) |
        puttable_black_diag(bd)) &
        ~stones(bd);
}

uint64_t makeLegalBoard(const board bd, const int player)
{
    const uint64_t player_board = player == 0 ? bd.black() : bd.white();
    const uint64_t opponent_board = player == 0 ? bd.white() : bd.black();

    //const int opponent = player ^ 1;

    // 空きマスのみにビットが立っているボード
    uint64_t blankBoard = empty_board & ~(player_board | opponent_board);

#if false
    board bd(player_board, opponent_board);
    return puttable_black(bd) & blankBoard;
#else
    // 左右端の番人
    uint64_t horizontalWatchBoard = opponent_board & 0x7e7e7e7e7e7e7e7e;
    // 上下端の番人
    uint64_t verticalWatchBoard = opponent_board & 0x00FFFFFFFFFFFF00;
    // 全辺の番人
    uint64_t allSideWatchBoard = opponent_board & 0x007e7e7e7e7e7e00;
    // 隣に相手の色があるかを一時保存する
    uint64_t tmp;
    // 返り値
    uint64_t legalBoard;

    // 8方向チェック
    //  ・一度に返せる石は最大6つ

    // 左
    tmp = horizontalWatchBoard & (player_board << 1);
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
    tmp = horizontalWatchBoard & (player_board >> 1);
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
    tmp = verticalWatchBoard & (player_board << 8);
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
    tmp = verticalWatchBoard & (player_board >> 8);
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
    tmp = allSideWatchBoard & (player_board << 7);
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
    tmp = allSideWatchBoard & (player_board << 9);
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
    tmp = allSideWatchBoard & (player_board >> 9);
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
    tmp = allSideWatchBoard & (player_board >> 7);
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
#endif
}

uint64_t moveOrderingTable_6x6[5] = {
    1ull << coordinateToIndex(0, 0) | 1ull << coordinateToIndex(5, 0) |
    1ull << coordinateToIndex(0, 5) | 1ull << coordinateToIndex(5, 5),

    1ull << coordinateToIndex(2, 0) | 1ull << coordinateToIndex(3, 0) |
    1ull << coordinateToIndex(0, 2) | 1ull << coordinateToIndex(5, 2) |
    1ull << coordinateToIndex(0, 3) | 1ull << coordinateToIndex(5, 3) |
    1ull << coordinateToIndex(2, 5) | 1ull << coordinateToIndex(3, 5),

    1ull << coordinateToIndex(2, 1) | 1ull << coordinateToIndex(3, 1) |
    1ull << coordinateToIndex(1, 2) | 1ull << coordinateToIndex(4, 2) |
    1ull << coordinateToIndex(1, 3) | 1ull << coordinateToIndex(4, 3) |
    1ull << coordinateToIndex(2, 4) | 1ull << coordinateToIndex(3, 4),

    1ull << coordinateToIndex(1, 0) | 1ull << coordinateToIndex(4, 0) |
    1ull << coordinateToIndex(0, 1) | 1ull << coordinateToIndex(5, 1) |
    1ull << coordinateToIndex(0, 4) | 1ull << coordinateToIndex(5, 4) |
    1ull << coordinateToIndex(1, 5) | 1ull << coordinateToIndex(4, 5),

    1ull << coordinateToIndex(1, 1) | 1ull << coordinateToIndex(4, 1) |
    1ull << coordinateToIndex(1, 4) | 1ull << coordinateToIndex(4, 4),
};

uint64_t moveOrderingTable_8x8[7] = {
    1ull << coordinateToIndex(0, 0) | 1ull << coordinateToIndex(7, 0) |
    1ull << coordinateToIndex(0, 7) | 1ull << coordinateToIndex(7, 7),

    1ull << coordinateToIndex(2, 0) | 1ull << coordinateToIndex(5, 0) |
    1ull << coordinateToIndex(0, 2) | 1ull << coordinateToIndex(7, 2) |
    1ull << coordinateToIndex(0, 5) | 1ull << coordinateToIndex(7, 5) |
    1ull << coordinateToIndex(2, 7) | 1ull << coordinateToIndex(5, 7),

    1ull << coordinateToIndex(3, 0) | 1ull << coordinateToIndex(4, 0) |
    1ull << coordinateToIndex(0, 3) | 1ull << coordinateToIndex(7, 3) |
    1ull << coordinateToIndex(0, 4) | 1ull << coordinateToIndex(7, 4) |
    1ull << coordinateToIndex(3, 7) | 1ull << coordinateToIndex(4, 7) |
    1ull << coordinateToIndex(3, 3) | 1ull << coordinateToIndex(4, 3) |
    1ull << coordinateToIndex(3, 4) | 1ull << coordinateToIndex(4, 4),

    1ull << coordinateToIndex(2, 2) | 1ull << coordinateToIndex(3, 2) | 1ull << coordinateToIndex(4, 2) | 1ull << coordinateToIndex(5, 2) |
    1ull << coordinateToIndex(2, 3) |                                                                     1ull << coordinateToIndex(5, 3) |
    1ull << coordinateToIndex(2, 4) |                                                                     1ull << coordinateToIndex(5, 4) |
    1ull << coordinateToIndex(2, 5) | 1ull << coordinateToIndex(3, 5) | 1ull << coordinateToIndex(4, 5) | 1ull << coordinateToIndex(5, 5),

    1ull << coordinateToIndex(2, 1) | 1ull << coordinateToIndex(3, 1) | 1ull << coordinateToIndex(4, 1) | 1ull << coordinateToIndex(5, 1) |
    1ull << coordinateToIndex(1, 2) | 1ull << coordinateToIndex(1, 3) | 1ull << coordinateToIndex(1, 4) | 1ull << coordinateToIndex(1, 5) |
    1ull << coordinateToIndex(6, 2) | 1ull << coordinateToIndex(6, 3) | 1ull << coordinateToIndex(6, 4) | 1ull << coordinateToIndex(6, 5) |
    1ull << coordinateToIndex(2, 6) | 1ull << coordinateToIndex(3, 6) | 1ull << coordinateToIndex(4, 6) | 1ull << coordinateToIndex(5, 6),

    1ull << coordinateToIndex(1, 0) | 1ull << coordinateToIndex(6, 0) |
    1ull << coordinateToIndex(0, 1) | 1ull << coordinateToIndex(7, 1) |
    1ull << coordinateToIndex(0, 6) | 1ull << coordinateToIndex(7, 6) |
    1ull << coordinateToIndex(1, 7) | 1ull << coordinateToIndex(6, 7),

    1ull << coordinateToIndex(1, 1) | 1ull << coordinateToIndex(6, 1) |
    1ull << coordinateToIndex(1, 6) | 1ull << coordinateToIndex(6, 6),
};

#if (COLUMNS == 4) && (ROWS == 4)
uint64_t moveOrdering[4 * 4 - 4] = {
    coordinateToIndex(1, 0),    // 1
    coordinateToIndex(0, 0),    // 2
    coordinateToIndex(0, 1),    // 3
    coordinateToIndex(2, 0),    // 4
    coordinateToIndex(3, 3),    // 5
    coordinateToIndex(0, 2),    // 6
    coordinateToIndex(3, 0),    // 7
    coordinateToIndex(1, 3),    // 8
    coordinateToIndex(0, 3),    // 9
    coordinateToIndex(2, 3),    // 10
    0,
    0
    //19, 18, 26, 20, 45, 34, 21, 43, 42, 44, 0, 0
};
#endif

#if (COLUMNS == 4) && (ROWS == 6)
uint64_t moveOrdering[4 * 6 - 4] = {
    coordinateToIndex(2, 0),    // 1
    coordinateToIndex(1, 0),    // 2
    coordinateToIndex(0, 0),    // 3
    coordinateToIndex(2, 3),    // 4
    coordinateToIndex(1, 1),    // 5
    coordinateToIndex(4, 0),    // 6
    coordinateToIndex(4, 3),    // 7
    coordinateToIndex(4, 1),    // 8
    coordinateToIndex(4, 2),    // 9
    coordinateToIndex(3, 0),    // 10
    coordinateToIndex(5, 0),    // 11
    coordinateToIndex(5, 3),    // 12
    coordinateToIndex(3, 3),    // 13
    coordinateToIndex(1, 2),    // 14
    coordinateToIndex(0, 3),    // 15
    coordinateToIndex(0, 1),    // 16
    coordinateToIndex(0, 2),    // 17
    coordinateToIndex(5, 1),    // 18
    coordinateToIndex(5, 2),    // 19
    coordinateToIndex(1, 3)     // 20
    //19, 18, 17, 43, 26, 21, 45, 29, 37, 20, 22, 46, 44, 34, 41, 25, 33, 30, 38, 42
};
#endif

#if (COLUMNS == 4) && (ROWS == 8)
uint64_t moveOrdering[4 * 8 - 4] = {
    coordinateToIndex(3, 0),    // 1
    coordinateToIndex(2, 0),    // 2
    coordinateToIndex(1, 0),    // 3
    coordinateToIndex(4, 0),    // 4
    coordinateToIndex(5, 0),    // 5
    coordinateToIndex(3, 3),    // 6
    coordinateToIndex(5, 3),    // 7
    coordinateToIndex(5, 1),    // 8
    coordinateToIndex(2, 3),    // 9
    coordinateToIndex(2, 1),    // 10
    coordinateToIndex(4, 3),    // 11
    coordinateToIndex(2, 2),    // 12
    coordinateToIndex(1, 1),    // 13
    coordinateToIndex(0, 0),    // 14
    coordinateToIndex(1, 3),    // 15
    coordinateToIndex(1, 2),    // 16
    coordinateToIndex(6, 3),    // 17
    coordinateToIndex(6, 0),    // 18
    coordinateToIndex(0, 1),    // 19
    coordinateToIndex(0, 3),    // 20
    coordinateToIndex(6, 1),    // 21
    coordinateToIndex(7, 3),    // 22
    coordinateToIndex(0, 2),    // 23
    coordinateToIndex(7, 1)     // 24
    //19, 18, 17, 20, 21, 29, 45, 26, 22, 44, 43, 30, 37, 23, 31, 39, 42, 41, 38, 33, 25, 24, 34, 0, 0, 0, 0, 0
};
#endif

#if (COLUMNS == 6) && (ROWS == 6)
uint64_t moveOrdering[6 * 6 - 4] = {
    coordinateToIndex(2, 1),    // 1    19
    coordinateToIndex(1, 3),    // 2    34
    coordinateToIndex(2, 4),    // 3    43
    coordinateToIndex(3, 1),    // 4    20
    coordinateToIndex(4, 3),    // 5    37
    coordinateToIndex(4, 2),    // 6    29
    coordinateToIndex(3, 0),    // 7    12
    coordinateToIndex(2, 0),    // 8    11
    coordinateToIndex(1, 0),    // 9    10
    coordinateToIndex(3, 4),    // 10   44
    coordinateToIndex(3, 5),    // 11   52
    coordinateToIndex(5, 3),    // 12   38
    coordinateToIndex(1, 2),    // 13   26
    coordinateToIndex(1, 1),    // 14   18
    coordinateToIndex(5, 2),    // 15   30
    coordinateToIndex(5, 1),    // 16   22
    coordinateToIndex(4, 1),    // 17   21
    coordinateToIndex(1, 5),    // 18   50
    coordinateToIndex(0, 3),    // 19   33
    coordinateToIndex(0, 2),    // 20   25
    coordinateToIndex(0, 4),    // 21   41
    coordinateToIndex(0, 5),    // 22   49
    coordinateToIndex(0, 0),    // 23   9
    coordinateToIndex(0, 1),    // 24   17
    coordinateToIndex(5, 0),    // 25   14
    coordinateToIndex(2, 5),    // 26   51
    coordinateToIndex(4, 0),    // 27   13
    coordinateToIndex(4, 5),    // 28   53
    coordinateToIndex(5, 4),    // 29   46
    coordinateToIndex(1, 4),    // 30   42
    coordinateToIndex(4, 4),    // 31   45
    coordinateToIndex(5, 5)     // 32   54
    //19, 34, 43, 20, 37, 29, 12, 11, 10, 44, 52, 38, 26, 18, 30, 22, 21, 50, 33, 25, 41, 49, 9, 17, 14, 51, 13, 53, 46, 42, 45, 54,
};
#endif

#if (COLUMNS == 8) && (ROWS == 8)
uint64_t moveOrdering[8 * 8 - 4] = {   // ウサギ定石→ローズ
    coordinateToIndex(5, 4),    // 1    37
    coordinateToIndex(3, 5),    // 2    43
    coordinateToIndex(2, 4),    // 3    34
    coordinateToIndex(5, 3),    // 4    29
    coordinateToIndex(4, 2),    // 5    20
    coordinateToIndex(2, 5),    // 6    42
    coordinateToIndex(3, 2),    // 7    19
    coordinateToIndex(5, 5),    // 8    45
    coordinateToIndex(4, 5),    // 9    44
    coordinateToIndex(3, 6),    // 10   51
    coordinateToIndex(6, 2),    // 11   22  シャープローズ定石
    coordinateToIndex(2, 3),    // 12   26
    coordinateToIndex(1, 3),    // 13   25
    coordinateToIndex(1, 2),    // 14   17
    coordinateToIndex(1, 5),    // 15   41
    coordinateToIndex(2, 2),    // 16   18
    coordinateToIndex(1, 4),    // 17   33
    coordinateToIndex(6, 3),    // 18   30
    59, 40, 31, 15, 16, 32, 53, 61, 13, 58, 57, 21, 38, 23, 7, 6, 4, 11, 3, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,S
    //progress(0 %) evaluation_value(-6)
    //37 43 34 29 20 42 19 45 44 51 22 26 25 17 41 18 33 30 59 40 31 15 16 32 53 61 13 58 57 21 38 23 7 6 4 11 3 8 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    //59, 40, 31, 58, 57, 12, 53, 62, 61, 23, 63, 39, 38, 47, 10, 21, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    //evaluation_value(5)
    //37 43 34 29 20 42 19 45 44 51 22 26 25 17 41 18 33 30 59 40 31 58 57 12 53 62 61 23 63 39 38 47 10 21 4 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
};
#endif

#ifndef __GNUC__
int GetNumberOfTrailingZeros(uint64_t x, const int hierarchy)
{
#if OPT_MOVE_ORDERING_ANSER
    if (hierarchy < sizeof(moveOrdering) / sizeof(moveOrdering[0]))
    {
        if (x & (1ull << moveOrdering[hierarchy])) {
            return moveOrdering[hierarchy];
        }
    }
#endif

#if OPT_MOVE_ORDERING_6x6
    int bit;
    if ((bit = std::countr_zero(x & moveOrderingTable_6x6[0])) != 64) {
        return bit;
    }
    if ((bit = std::countr_zero(x & moveOrderingTable_6x6[1])) != 64) {
        return bit;
    }
    if ((bit = std::countr_zero(x & moveOrderingTable_6x6[2])) != 64) {
        return bit;
    }
    if ((bit = std::countr_zero(x & moveOrderingTable_6x6[3])) != 64) {
        return bit;
    }
    if ((bit = std::countr_zero(x & moveOrderingTable_6x6[4])) != 64) {
        return bit;
    }
    return std::countr_zero(x);
#elif OPT_MOVE_ORDERING_8x8
    int bit;
    if ((bit = std::countr_zero(x & moveOrderingTable_8x8[0])) != 64) {
        return bit;
    }
    if ((bit = std::countr_zero(x & moveOrderingTable_8x8[1])) != 64) {
        return bit;
    }
    if ((bit = std::countr_zero(x & moveOrderingTable_8x8[2])) != 64) {
        return bit;
    }
    if ((bit = std::countr_zero(x & moveOrderingTable_8x8[3])) != 64) {
        return bit;
    }
    if ((bit = std::countr_zero(x & moveOrderingTable_8x8[4])) != 64) {
        return bit;
    }
    if ((bit = std::countr_zero(x & moveOrderingTable_8x8[5])) != 64) {
        return bit;
    }
    if ((bit = std::countr_zero(x & moveOrderingTable_8x8[6])) != 64) {
        return bit;
    }
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
