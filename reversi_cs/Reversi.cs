using System.Numerics;

namespace reversi
{
    internal class Reversi
    {
        // 行列(x,y)
        public static int rows;
        public static int columns;
        static int simulation_hierarchy_max = 4;

        // 手番
        public enum ePLAYER { P0 = 0, P1, NUM };
        ePLAYER player = ePLAYER.P0;
        public ePLAYER Player { get { return player; } }

        // ePLAYER.P0とePLAYER.P1が置いたセルを表すbitmask
        ulong[] board = new ulong[(int)ePLAYER.NUM];

        // ePLAYER.P0とePLAYER.P1が置けるセルを表すbitmask
        ulong[] placeable = new ulong[(int)ePLAYER.NUM];

        // 両者置けるセルがなくなったときtrue
        bool isGameOver = false;

        public CResult[,] result = new CResult[8, 8];


        public Reversi(int row, int column)
        {
            // 盤面の数
            rows = row;
            columns = column;

            for (int y = 0; y < columns; y++) {
                for (int x = 0; x < rows; x++) {
                    int put_bit = coordinateToIndex(x, y);
                    ulong put_mask = 1ul << put_bit;
                    empty_board |= put_mask;
                }
            }

            // 先手
            player = ePLAYER.P0;

            // 先手（黒）
            board[0] = 1ul << coordinateToIndex(rows / 2 - 0, columns / 2 - 1) |
                       1ul << coordinateToIndex(rows / 2 - 1, columns / 2 - 0);
            // 後手（白）
            board[1] = 1ul << coordinateToIndex(rows / 2 - 1, columns / 2 - 1) |
                       1ul << coordinateToIndex(rows / 2 - 0, columns / 2 - 0);

            // 置ける位置を調べる
            PlaceableTest();

            isGameOver = false;
        }

        int GetNumberOfTrailingZeros(ulong x)
        {
            return BitOperations.TrailingZeroCount(x);
        }

        CResult simulationSingle(ulong[] board, int player, int hierarchy_max)
        {
            CResult result = new CResult();

            if (hierarchy_max <= 0) {
                result.set(board);
                return result;
            }

            ulong legalBoard = makeLegalBoard(board, player);

            if (legalBoard != 0ul) {
                int opponent = player ^ 1;
                ulong m = legalBoard;
                int bit;
                while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
                    ulong[] temp_board = { board[0], board[1] };
                    reverse(1ul << bit, temp_board, player);
                    result.marge(simulationSingle(temp_board, opponent, hierarchy_max - 1));

                    m &= ~(1ul << bit);
                }
            } else {
                player ^= 1;

                legalBoard = makeLegalBoard(board, player);

                if (legalBoard != 0ul) {
                    int opponent = player ^ 1;
                    ulong m = legalBoard;
                    int bit;
                    while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
                        ulong[] temp_board = { board[0], board[1] };
                        reverse(1ul << bit, temp_board, player);
                        result.marge(simulationSingle(temp_board, opponent, hierarchy_max - 1));

                        m &= ~(1ul << bit);
                    }
                } else {
                    result.set(board);
                }
            }

            return result;
        }

        private static bool bitTest(int x, int y, ulong mask)
        {
            int piece_bit = coordinateToIndex(x, y);
            ulong piece_mask = 1ul << piece_bit;
            return (mask & piece_mask) == piece_mask;
        }

        // playerが置いているセルか調べる
        public bool IsExist(int x, int y, ePLAYER player)
        {
            return bitTest(x, y, board[(int)player]);
        }

        // playerが置けるセルか調べる
        public bool IsPlaceable(int x, int y, ePLAYER player)
        {
            return bitTest(x, y, placeable[(int)player]);
        }

        // 置ける位置を調べる
        public int PlaceableTest()
        {
            placeable[0] = placeable[1] = 0;

            placeable[(int)player] = makeLegalBoard(board, (int)player);

            for (int x = 0; x < rows; x++) {
                for (int y = 0; y < rows; y++) {
                    int opponent = (int)player ^ 1;
                    ulong[] temp_board = { board[0], board[1] };
                    int bit = coordinateToIndex(x, y);
                    reverse(1ul << bit, temp_board, (int)player);
                    result[x, y] = simulationSingle(temp_board, opponent, simulation_hierarchy_max);
                }
            }

            // 置けるセルの数を返す
            return GetBitCount(placeable[(int)player]);
        }

        // xとyで指定したセルに置けるか調べる
        public bool IsPlaceable(int x, int y)
        {
            ulong legalBoard = makeLegalBoard(board, (int)player);
            return bitTest(x, y, legalBoard);
        }

        // xとyで指定したセルに置く
        public void Place(int x, int y)
        {
            int put_bit = coordinateToIndex(x, y);
            ulong put_mask = 1ul << put_bit;
            reverse(put_mask, board, (int)player);

            player = player == ePLAYER.P0 ? ePLAYER.P1 : ePLAYER.P0;

            if (0 == PlaceableTest()) {
                player = player == ePLAYER.P0 ? ePLAYER.P1 : ePLAYER.P0;

                if (0 == PlaceableTest()) {
                    isGameOver = true;
                }
            }
        }

        public int Places(ePLAYER player)
        {
            return GetBitCount(board[(int)player]);
        }

        public static int GetBitCount(ulong x)
        {
            return BitOperations.PopCount(x);
        }

        // x, y から通し番号を得る
        private static int coordinateToIndex(int x, int y)
        {
            //return y * rows + x;
            return y * 8 + x;
        }

        void reverse(ulong put_mask, ulong[] board, int player)
        {
            int opponent = player ^ 1;

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

        public static ulong transfer(ulong put, int k)
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

        ulong empty_board = 0;

        ulong makeLegalBoard(ulong[] board, int player)
        {
            int opponent = player ^ 1;

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
    }
}
