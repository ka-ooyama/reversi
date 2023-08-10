#ifndef DEFINE_H
#define DEFINE_H

#define COLUMNS 8  // 縦のマス数
#define ROWS 8    // 横のマス数

// 総手数 = COLUMNS * ROWS - 4(初期配置)
#define TURNS (COLUMNS * ROWS - 4)

// （6x6を最後まで解析するのは時間がかかりすぎるので）初期の解析など途中で切り上げたいときは
#if true  // ここを true にして実行したい階層数を書く
#undef TURNS
#define TURNS 16
static_assert((TURNS < COLUMNS* ROWS - 4), "TURNS is too large.");
#endif

// （先頭から）シングルスレッドで処理する階層数
#define SINGLE_HIERARCHEY_TOP 0
// （末端から）シングルスレッドで処理する階層数
#define SINGLE_HIERARCHEY_BTM 10

// キャッシュを有効化する階層の深さ
// 枝刈り率が低ければ検索のコストのほうが高く付く（6x6で見る限り有効にしないほうがいい）
#define CACHED_HIERARCHEY 15

// 試行を繰り返す回数
// 4x4など早すぎて正確に計測できないとき複数回まわして平均をとる
#define NUMBER_OF_TRIALS 1

// 使用するスレッドの数，大きな値を指定しても使用しているCPUの最大値以上にはならない
#define WORKER_THREAD_MAX 20

// アルファベータ法で枝刈りする
#define OPT_ALPHA_BETA true
// 行と列が偶数かつ同じとき１手目を省略する（４か所がすべてが同じ結果になるため）
// 4x4, 4x6, 4x8, 4x10 を比較する際などは false にする（または4x4の結果を4倍して比較する必要がある）
#define OPT_TETRAGONALITY true

// 6x6向きのムーブオーダリングテーブルを使う
#define OPT_MOVE_ORDERING_6x6 true
// １手目に（解析済みの）最善手を打つ（4x4,4x6,4x8,6x6のみ対応）
#define OPT_MOVE_ORDERING_ANSER true

// 計測に影響が出るようなデバッグ表示を消すための定義
// 普段はtrueにしておいたほうが確認しやすいけれども最終的な計測はfalseで行う
#define DEBUG_PRINT true
// （枝刈りされずに）処理された盤面の数
#define PRINT_NODES false

#endif  // DEFINE_H
