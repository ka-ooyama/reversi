﻿using reversi;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;

namespace reversi
{
    internal class CResult
    {
        public CResult()
        { }
        ~CResult() { }

        public void set(ulong[] board)
        {
            int board0 = BitOperations.PopCount(board[0]);
            int board1 = BitOperations.PopCount(board[1]);

            high = low = (int)board0 - (int)board1;

            if (board0 > board1) {
                win = 1;
            } else if (board0 < board1) {
                lose = 1;
            } else {
                draw = 1;
            }
        }

        public void marge(CResult result)
        {
            high = Math.Max(high, result.high);
            low = Math.Min(low, result.low);
            win += result.win;
            lose += result.lose;
            draw += result.draw;
        }

        public int winRate(int player)
        {
            int match = win + lose + draw;
            int rate = (int)(((double)win + 0.5 * (double)draw) / (double)match * 100.0);
            return player == 0 ? rate : 100 - rate;
        }

        void print()
        {
            //printf("win(%8d) lose(%8d) draw(%8d) high(%+3d) low(%+3d)\n", win, lose, draw, high, low);
        }

        int high = int.MinValue;
        int low = int.MaxValue;
        int win = 0;
        int lose = 0;
        int draw = 0;
    }
}
