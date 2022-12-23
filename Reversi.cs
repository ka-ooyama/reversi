using Microsoft.VisualBasic.ApplicationServices;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Numerics;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using static reversi.Reversi;
using static System.Windows.Forms.VisualStyles.VisualStyleElement;

namespace reversi
{
    internal class Reversi
    {
        // 行と列（本当は8x8だが6x6でテスト中）
        static int rows = 6;
        static int columns = 6;

        static int yNum = 6;
        static int xNum = 6;

        // 手番
        public enum ePLAYER { P0 = 0, P1, NUM };
        ePLAYER player = ePLAYER.P0;
        public ePLAYER Player { get { return player; } }

        // ePLAYER.P0とePLAYER.P1が置いたセルを表すbitmask
        UInt64[] piece = new UInt64[(int)ePLAYER.NUM];
        // ePLAYER.P0とePLAYER.P1が置けるセルを表すbitmask
        UInt64[] placeable = new UInt64[(int)ePLAYER.NUM];
        // 両者置けるセルがなくなったときtrue
        bool isGameOver = false;

        public int aaa = 0;

        public Reversi(int col, int row)
        {
            // 盤面の数
            yNum = rows = row;
            xNum = columns = col;

            // 先手
            player = ePLAYER.P0;

            // 先手（黒）
            piece[0] = 1ul << coordinateToIndex(rows / 2 - 1, columns / 2 - 1) |
                       1ul << coordinateToIndex(rows / 2 - 0, columns / 2 - 0);
            // 後手（白）
            piece[1] = 1ul << coordinateToIndex(rows / 2 - 0, columns / 2 - 1) |
                       1ul << coordinateToIndex(rows / 2 - 1, columns / 2 - 0);

            // 置ける位置を調べる
            PlaceableTest();

            isGameOver = false;

            //aaa = simulation(piece[0], piece[1]);
        }

        private static int simulation(UInt64 piece_player, UInt64 piece_rybal)
        {
            int num = 0;

            UInt64 legalBoard = makeLegalBoard(piece_player, piece_rybal);

            for (int y = 0; y < yNum; y++)
            {
                for (int x = 0; x < xNum; x++)
                {
                    if (bitTest(x, y, legalBoard))
                    {
                        UInt64 temp_player = piece_player;
                        UInt64 temp_rybal = piece_rybal;
                        Place(x, y, ref temp_player, ref temp_rybal);
                        num += simulation(temp_rybal, temp_player);
                    }
                }
            }

            if (num == 0)
            {
                UInt64 tmp = piece_player;
                piece_player = piece_rybal;
                piece_rybal = tmp;

                legalBoard = makeLegalBoard(piece_player, piece_rybal);

                for (int y = 0; y < yNum; y++)
                {
                    for (int x = 0; x < xNum; x++)
                    {
                        if (bitTest(x, y, legalBoard))
                        {
                            UInt64 temp_player = piece_player;
                            UInt64 temp_rybal = piece_rybal;
                            Place(x, y, ref temp_player, ref temp_rybal);
                            num += simulation(temp_rybal, temp_player);
                        }
                    }
                }
            }

            return num == 0 ? 1 : num;
            // 置けるセルの数を返す
            //return GetBitCount(placeable[(int)player]);
        }

        private static bool bitTest(int x, int y, UInt64 mask)
        {
            int piece_bit = coordinateToIndex(x, y);
            UInt64 piece_mask = 1ul << piece_bit;
            return (mask & piece_mask) == piece_mask;
        }

        // playerが置いているセルか調べる
        public bool IsExist(int x, int y, ePLAYER player)
        {
            return bitTest(x, y, piece[(int)player]);
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

            ePLAYER rybal = player == ePLAYER.P0 ? ePLAYER.P1 : ePLAYER.P0;
            placeable[(int)player] = makeLegalBoard(piece[(int)player], piece[(int)rybal]);

            // 置けるセルの数を返す
            return GetBitCount(placeable[(int)player]);
        }

        // xとyで指定したセルに置けるか調べる
        public bool IsPlaceable(int x, int y)
        {
            ePLAYER rybal = player == ePLAYER.P0 ? ePLAYER.P1 : ePLAYER.P0;
            UInt64 legalBoard = makeLegalBoard(piece[(int)player], piece[(int)rybal]);
            return bitTest(x, y, legalBoard);
        }

        private static void Place(int x, int y, ref UInt64 piece_player, ref UInt64 piece_rybal)
        {
            int piece_bit = coordinateToIndex(x, y);
            UInt64 piece_mask = 1ul << piece_bit;

            // 着手した場合のボードを生成
            UInt64 rev = 0;
            for (int i = 0; i < 8; i++)
            {
                UInt64 rev_ = 0;
                UInt64 mask = transfer(piece_mask, i);
                while ((mask != 0) && ((mask & piece_rybal) != 0))
                {
                    rev_ |= mask;
                    mask = transfer(mask, i);
                }
                if ((mask & piece_player) != 0)
                {
                    rev |= rev_;
                }
            }

            // 反転する
            piece_player ^= piece_mask | rev;
            piece_rybal ^= rev;
        }

        // xとyで指定したセルに置く
        public void Place(int x, int y)
        {
            ePLAYER rybal = player == ePLAYER.P0 ? ePLAYER.P1 : ePLAYER.P0;

            int piece_bit = coordinateToIndex(x, y);
            UInt64 piece_mask = 1ul << piece_bit;

            // 着手した場合のボードを生成
            UInt64 rev = 0;
            for (int i = 0; i < 8; i++)
            {
                UInt64 rev_ = 0;
                UInt64 mask = transfer(piece_mask, i);
                while ((mask != 0) && ((mask & piece[(int)rybal]) != 0))
                {
                    rev_ |= mask;
                    mask = transfer(mask, i);
                }
                if ((mask & piece[(int)player]) != 0)
                {
                    rev |= rev_;
                }
            }

            // 反転する
            piece[(int)player] ^= piece_mask | rev;
            piece[(int)rybal] ^= rev;

            player = player == ePLAYER.P0 ? ePLAYER.P1 : ePLAYER.P0;

            if (0 == PlaceableTest())
            {
                player = player == ePLAYER.P0 ? ePLAYER.P1 : ePLAYER.P0;

                if (0 == PlaceableTest())
                {
                    isGameOver = true;
                }
            }
        }

        public int Places(ePLAYER player)
        {
            return GetBitCount(piece[(int)player]);
        }

#if false
        public static int GetBitCount(int x)
        {
            x = (x & 0x55555555) + ((x >> 1) & 0x55555555);
            x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
            x = (x & 0x0F0F0F0F) + ((x >> 4) & 0x0F0F0F0F);
            x = (x & 0x00FF00FF) + ((x >> 8) & 0x00FF00FF);
            x = (x & 0x0000FFFF) + ((x >> 16) & 0x0000FFFF);
            return (int)x;
        }
#endif

        public static int GetBitCount(UInt64 x)
        {
            return BitOperations.PopCount(x);
        }

        // x, y から通し番号を得る
        private static int coordinateToIndex(int x, int y)
        {
            //return y * rows + x;
            return y * 8 + x;
        }

        public static UInt64 transfer(UInt64 put, int k)
        {
            switch (k)
            {
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

        public static UInt64 makeLegalBoard(UInt64 piece_player, UInt64 piece_rybal)
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
    }
}
