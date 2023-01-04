#ifndef DEFINE_H
#define DEFINE_H

#define COLUMNS					4   // 縦のマス数
#define ROWS					6	// 横のマス数

// 総手数 = COLUMNS * ROWS - 4(初期配置)

// 予め打っておく手数（最大値：総手数）
#define PRESET_HIERARCHEY		0	// 6x6ですぐ終わるのは15前後

// 末端から数えてマルチスレッド化しない階層数
#define HIERARCHEY_SINGLE		3

// マルチスレッドでキャッシュを検索する階層の深さ（最大値：総手数）
// 大きくしすぎるとメモリが足りず仮想メモリに割り当てられて遅くなる
#define HIERARCHEY_CACHED		0

// 試行を繰り返す回数
// 4x4など早すぎて正確に計測できないとき複数回まわして平均をとる
#define NUMBER_OF_TRIALS		1

// キャッシュ検索に対称性を利用する（8パターンを同じとみなすことでキャッシュ効率をあげる）
#define USE_SYMMETRY_OPTIMIZE   false   // 未完成 false にしておくこと

// 計測に影響が出るようなデバッグ表示を消すための定義
// 普段はtrueにしておいたほうが確認しやすいけれども最終的な計測はfalseで行う
#define DEBUG_PRINT				true

// 使用するスレッドの数，大きな値を指定しても使用しているCPUの最大値以上にはならない
#define WORKER_THREAD_MAX		20

#define ANALYZE_NODE_HIERARCHEY false

// アルファベータ法で枝刈りする
#define ALPHA_BETA              true

// 6x6向きのムーブオーダリングテーブルを使う
#define MOVE_ORDERING_6x6       true

#endif  // DEFINE_H
