using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;

namespace reversi
{
    internal class Reversi
    {
        // 行と列（本当は8x8だが6x6でテスト中）
        int rows = 6;
        int columns = 6;

        int yNum = 6;
        int xNum = 6;

        // 8方向の検索用ベクトル
        Point[] searchVec = {
            new Point( -1, -1 ),
            new Point( +0, -1 ),
            new Point( +1, -1 ),
            new Point( -1, +0 ),
            new Point( +1, +0 ),
            new Point( -1, +1 ),
            new Point( +0, +1 ),
            new Point( +1, +1 )
        };

        // 手番
        public enum ePLAYER { P0 = 0, P1, NUM };
        ePLAYER player = ePLAYER.P0;
        public ePLAYER Player { get { return player; } }

        // ePLAYER.P0とePLAYER.P1が置いたセルを表すbitmask
        ulong[] piece = new ulong[(int)ePLAYER.NUM];
        // ePLAYER.P0とePLAYER.P1が置けるセルを表すbitmask
        ulong[] placeable = new ulong[(int)ePLAYER.NUM];
        // 両者置けるセルがなくなったときtrue
        bool isGameOver = false;

        public Reversi(int col, int row)
        {
            // 盤面の数
            yNum = rows = row;
            xNum = columns = col;

            // 先手
            player = ePLAYER.P0;

            // 先手（白）
            piece[0] =
                1ul << cellToIndex(rows / 2 - 1, columns / 2 - 1) |
                1ul << cellToIndex(rows / 2 - 0, columns / 2 - 0);
            // 後手（黒）
            piece[1] =
                1ul << cellToIndex(rows / 2 - 0, columns / 2 - 1) |
                1ul << cellToIndex(rows / 2 - 1, columns / 2 - 0);

            // 置ける位置を調べる
            PlaceableTest();

            isGameOver = false;
        }

        // playerが置いているセルか調べる
        public bool IsExist(int x, int y, ePLAYER player)
        {
            int piece_bit = cellToIndex(x, y);
            ulong piece_mask = 1ul << piece_bit;

            return (piece[(int)player] & piece_mask) == piece_mask;
        }

        // playerが置けるセルか調べる
        public bool IsPlaceable(int x, int y, ePLAYER player)
        {
            int piece_bit = cellToIndex(x, y);
            ulong piece_mask = 1ul << piece_bit;

            return (placeable[(int)player] & piece_mask) == piece_mask;
        }

        // 置ける位置を調べる
        public int PlaceableTest()
        {
            placeable[0] = placeable[1] = 0;

            for (int y = 0; y < yNum; y++)
            {
                for (int x = 0; x < xNum; x++)
                {
                    if (IsPlaceable(x, y))
                    {
                        int piece_bit = cellToIndex(x, y);
                        ulong piece_mask = 1ul << piece_bit;

                        placeable[(int)player] |= piece_mask;
                    }
                }
            }

            // 置けるセルの数を返す
            return GetBitCount(placeable[(int)player]);
        }

        // xとyで指定したセルに置けるか調べる
        public bool IsPlaceable(int x, int y)
        {
            ePLAYER rybal = player == ePLAYER.P0 ? ePLAYER.P1 : ePLAYER.P0;

            // playerまたはrybalがすでに置いているセル
            if (IsExist(x, y, player) || IsExist(x, y, rybal))
            {
                return false;
            }

            // 8方向にそれぞれに繰り返す
            foreach (var v in searchVec)
            {
                int step = 0;   // 検索の状態 （0:挟まれる相手の駒を探索中，1:挟む自分の駒を探索中）

                for (int dx = x + v.X, dy = y + v.Y; 0 <= dx && dx < xNum && 0 <= dy && dy < yNum; dx += v.X, dy += v.Y)
                {
                    if (step == 0)
                    {
                        if (IsExist(dx, dy, rybal))
                        {
                            step++;
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                    else if (step == 1)
                    {
                        if (IsExist(dx, dy, player))
                        {
                            return true;
                        }
                        else if (IsExist(dx, dy, rybal))
                        {
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }

            return false;
        }

        // xとyで指定したセルに置く
        public void Place(int x, int y)
        {
            ePLAYER rybal = player == ePLAYER.P0 ? ePLAYER.P1 : ePLAYER.P0;

            int piece_bit = cellToIndex(x, y);
            ulong piece_mask = 1ul << piece_bit;

            piece[(int)player] |= piece_mask;

            foreach (var v in searchVec)
            {
                int step = 0;

                for (int dx = x + v.X, dy = y + v.Y; 0 <= dx && dx < xNum && 0 <= dy && dy < yNum; dx += v.X, dy += v.Y)
                {
                    if (step == 0)
                    {
                        if (IsExist(dx, dy, rybal))
                        {
                            step++;
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                    else if (step == 1)
                    {
                        if (IsExist(dx, dy, player))
                        {
                            while (0 <= dx && dx < xNum && 0 <= dy && dy < yNum && !(dx == x && dy == y))
                            {
                                piece_bit = cellToIndex(dx, dy);
                                piece_mask = 1ul << piece_bit;

                                piece[(int)player] |= piece_mask;
                                piece[(int)rybal] = (piece[(int)rybal] & ~piece_mask);

                                dx -= v.X;
                                dy -= v.Y;
                            }
                            break;
                        }
                        else if (IsExist(dx, dy, rybal))
                        {
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }

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

        public static int GetBitCount(ulong x)
        {
            return BitOperations.PopCount(x);
        }

        // x, y から通し番号を得る
        int cellToIndex(int x, int y)
        {
            return y * rows + x;
        }
    }
}
