#ifndef DEFINE_H
#define DEFINE_H

#define COLUMNS					4   // 縦のマス数
#define ROWS					6   // 横のマス数

// 総手数 = COLUMNS * ROWS - 4(初期配置)

#define STOP_HIERARCHEY		    64  // (default : 64) 途中で切り上げる手数（6x6は時間がかかりすぎるので16程度で切り上げて初期解析する）

#define HIERARCHEY_SINGLE		3   // 末端から数えてマルチスレッド化しない階層数

// マルチスレッドでキャッシュを有効化する階層の深さ
// 枝刈り率が低ければ検索のコストのほうが高く付く（6x6で見る限り有効にしないほうがいい）
#define CACHED_HIERARCHEY		15

// 試行を繰り返す回数
// 4x4など早すぎて正確に計測できないとき複数回まわして平均をとる
#define NUMBER_OF_TRIALS		1

// 使用するスレッドの数，大きな値を指定しても使用しているCPUの最大値以上にはならない
#define WORKER_THREAD_MAX		20

#define OPT_CACHE               false   // 結果をキャッシュする
#define OPT_SYMMETRY            false   // キャッシュ検索に対称性を利用する（8パターンを同じとみなすことでキャッシュ効率をあげる）
#define OPT_MOVE_ORDERING_6x6   true    // 6x6向きのムーブオーダリングテーブルを使う
#define OPT_ALPHA_BETA          true    // アルファベータ法で枝刈りする

// 計測に影響が出るようなデバッグ表示を消すための定義
// 普段はtrueにしておいたほうが確認しやすいけれども最終的な計測はfalseで行う
#define DEBUG_PRINT				true
#define CACHE_ANALYZE           true    // キャッシュのヒット率を表示する

#endif  // DEFINE_H
