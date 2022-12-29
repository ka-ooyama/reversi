using System.Numerics;

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

            if (board0 > board1) {
                win = 1;
                high = low = Reversi.rows * Reversi.columns - (int)board1;
            } else if (board0 < board1) {
                lose = 1;
                high = low = (int)board0 - Reversi.rows * Reversi.columns;
            } else {
                draw = 1;
                high = low = 0;
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
            ulong match = win + lose + draw;
            int rate = (int)(((double)win + 0.5 * (double)draw) / (double)match * 100.0);
            return player == 0 ? rate : 100 - rate;
        }

        void print()
        {
            //printf("win(%8d) lose(%8d) draw(%8d) high(%+3d) low(%+3d)\n", win, lose, draw, high, low);
        }

        int high = int.MinValue;
        int low = int.MaxValue;
        ulong win = 0;
        ulong lose = 0;
        ulong draw = 0;
    }
}
