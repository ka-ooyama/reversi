#ifndef __GNUC__
#include <bit>
#endif
#include <bitset>
#include <cstdint>
#include <future>
#include <mutex>
#include <thread>
#include <vector>
#include <concurrent_unordered_map.h>

using namespace concurrency;

//////////////// 以下を貼る ////////////////
template<class T> size_t HashCombine(const size_t seed, const T& v)
{
	return seed ^ (std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}
/* pair用 */
template<class T, class S> struct std::hash<std::pair<T, S>> {
	size_t operator()(const std::pair<T, S>& keyval) const noexcept
	{
		return HashCombine(std::hash<T>()(keyval.first), keyval.second);
	}
};
////////////////////////////////////////////

// CPUの並列度（△コア，〇スレッドの〇）
uint32_t hardware_concurrency = std::thread::hardware_concurrency();

#include "define.h"

// 行列(columns x rows)
const int columns = COLUMNS;
const int rows = ROWS;

const int hierarchy_single = HIERARCHEY_SINGLE;
const int hierarchy_cached = HIERARCHEY_CACHED;

const int number_of_trials = NUMBER_OF_TRIALS;

const uint32_t worker_threads_num = std::clamp<uint32_t>(WORKER_THREAD_MAX, 0, hardware_concurrency);

// x, y から通し番号を得る
int coordinateToIndex(const int x, const int y) { return y * 8 + x; }

#include "bit_util.h"
#include "node.h"
#include "result.h"

struct MyTimer {
	clock_t start;
	MyTimer() { start = clock(); }
	virtual ~MyTimer()
	{
		clock_t end = clock();
		printf("経過時間 = %fsec.\n", (double)(end - start) / CLOCKS_PER_SEC);
	}
};

enum ePLAYER { ePLAYER_P0 = 0, ePLAYER_P1, NUM };

void simulationPush(CNode* node);
CResult simulationSingle(const uint64_t board[], int player, const int hierarchy);

void reverse(const uint64_t put_mask, uint64_t board[], const int player);
uint64_t transfer(const uint64_t put, const int k);
uint64_t makeLegalBoard(const uint64_t board[], const int player);

concurrent_unordered_map<std::pair<uint64_t, uint64_t>, CResult> result_cache[2][36];

std::mutex mutex;
std::vector<CNode*> jobs;
uint32_t initial_jobs_num = 1;      // 総job数
std::vector<std::thread> threads;   // ワーカースレッド
#if DEBUG_PRINT
std::atomic<uint64_t> final_num{ 0 };
#endif

void worker(void)
{
	while (!jobs.empty()) {
		int worker_id = 0;

		CNode* node;
		{
			std::lock_guard<std::mutex> lock(mutex);
			if (jobs.empty()) {
				break;
			}

			worker_id = (int)jobs.size();

			node = jobs.back();

			jobs.pop_back();
		}
		CResult result = simulationSingle(node->boardPointer(), node->player(), node->hierarchy());

#if DEBUG_PRINT
		result.print((int)(worker_id * 100.0f / initial_jobs_num));
#endif
	}
}

int main(void)
{
	initialize_bit_util();

	uint64_t board[] = { 0, 0 };

	// 先手
	ePLAYER player = ePLAYER_P0;

	// 先手（黒）
	board[0] =
		1ul << coordinateToIndex(rows / 2 - 0, columns / 2 - 1) |
		1ul << coordinateToIndex(rows / 2 - 1, columns / 2 - 0);
	// 後手（白）
	board[1] =
		1ul << coordinateToIndex(rows / 2 - 1, columns / 2 - 1) |
		1ul << coordinateToIndex(rows / 2 - 0, columns / 2 - 0);

#if false  // 一つ打つ
	// 後手
	player = ePLAYER_P1;

	// 先手（黒）
	board[0] =
		1ull << coordinateToIndex(rows / 2 - 2, columns / 2 - 1) |
		1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 1) |
		1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 1) |
		1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 0);
	// 後手（白）
	board[1] =
		1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 0);
#endif

#if false  // 一つ打つ
	// 後手
	player = ePLAYER_P0;

	// 先手（黒）
	board[0] =
		1ull << coordinateToIndex(rows / 2 - 2, columns / 2 - 1) |
		1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 1) |
		1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 0);
	// 後手（白）
	board[1] =
		1ull << coordinateToIndex(rows / 2 - 2, columns / 2 - 2) |
		1ull << coordinateToIndex(rows / 2 - 1, columns / 2 - 1) |
		1ull << coordinateToIndex(rows / 2 - 0, columns / 2 - 0);
#endif

	uint64_t scale = 1;

	for (int i = 0; i < PRESET_HIERARCHEY; i++)
	{
		uint64_t legalBoard = makeLegalBoard(board, player);
		if (legalBoard != 0ull) {
			const int opponent = player ^ 1;
			size_t legalBoardNum = std::bitset<64>(legalBoard).count();
			scale *= legalBoardNum;
			uint64_t m = legalBoard;
			int bit = GetNumberOfTrailingZeros(m);
			if (bit != 64) {
				reverse(1ull << bit, board, player);
			}
		}
		player = player == ePLAYER_P0 ? ePLAYER_P1 : ePLAYER_P0;
	}

	printf("[%d x %d]\n", columns, rows);
	printf("論理プロセッサ   %d\n", hardware_concurrency);
	printf("ワーカースレッド %d\n", worker_threads_num);
	printf("総階層数 %d\n", columns * rows - 4);
	printf("%d階層目からマルチスレッド化する\n", hierarchy_single);
	printf("%d階層目まで結果をキャッシュする\n", hierarchy_cached);
	printf("対称形を省いて最適化する %s\n", USE_SYMMETRY_OPTIMIZE ? "true" : "false");
#if DEBUG_PRINT
	printf("\n");
#endif

	{
		MyTimer myTimer;

		for (int i = 0; i < number_of_trials; i++)
		{
#if DEBUG_PRINT
			final_num = 0;
#endif

			for (int i = 0; i < 2; i++)
			{
				for (int j = 0; j < 36; j++)
				{
					result_cache[i][j].clear();
				}

				CNode::jobs_[i].clear();

				CNode::nodes_[i].clear();

			}

			jobs.clear();

			if (hierarchy_single == 0) {
				CResult result = simulationSingle(board, (int)player, 0);
#if DEBUG_PRINT
				result.print(0);
#endif
			}
			else {
				CNode* root = new CNode(board, (int)player, 0);

				simulationPush(root);

				for (int i = 0; i < 2; i++)
				{
					for (auto it = CNode::jobs_[i].begin(), e = CNode::jobs_[i].end(); it != e; ++it) {
						CNode* p = it->second;
						jobs.push_back(p);
					}
				}

				initial_jobs_num = (uint32_t)jobs.size();

				for (uint32_t i = 0; i < worker_threads_num; i++) {
					threads.emplace_back(std::thread(worker));
				}

				for (auto&& th : threads) {
					th.join();
				}

				threads.clear();
			}
		}
	}

	printf("\n");
	printf("予想倍率 %llu(%d^%lf)\n", scale, PRESET_HIERARCHEY, log((double)scale) / log((double)PRESET_HIERARCHEY) );
	printf("総ジョブ数 %3d\n", initial_jobs_num);
	//printf("キャッシュ総数 %llu\n", result_cache[0].size() + result_cache[1].size() + CNode::nodes_[0].size() + CNode::nodes_[1].size());
#if DEBUG_PRINT
	printf("最終局面 %llu\n", final_num.load());
#endif

	return 0;
}

#if false
void simulationPush(CNode* node)
{
	const uint64_t* board = node->boardPointer();
	int player = node->player();

	uint64_t legalBoard = makeLegalBoard(board, player);

	if (legalBoard != 0ull) {
		const int opponent = player ^ 1;
		size_t legalBoardNum = std::bitset<64>(legalBoard).count();
		bool is_push =
			(node->hierarchy() + 1) >= hierarchy_single || legalBoardNum == 1;
		uint64_t m = legalBoard;
		int bit;
		while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
			uint64_t temp_board[2] = { board[0], board[1] };
			reverse(1ull << bit, temp_board, player);
			CNode* child = node->addChild(temp_board, opponent, is_push);
			m &= ~(1ull << bit);
		}

		if (!is_push) {
			for (std::vector<CNode*>::const_iterator i =
				node->getChildren().begin();
				i != node->getChildren().end(); i++) {
				CNode* child = *i;
				simulationPush(child);
			}
		}
	}
	else {
		player ^= 1;

		legalBoard = makeLegalBoard(board, player);

		if (legalBoard != 0ull) {
			const int opponent = player ^ 1;
			size_t legalBoardNum = std::bitset<64>(legalBoard).count();
			bool is_push =
				(node->hierarchy() + 1) >= hierarchy_single || legalBoardNum == 1;
			uint64_t m = legalBoard;
			int bit;
			while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
				uint64_t temp_board[2] = { board[0], board[1] };
				reverse(1ull << bit, temp_board, player);
				CNode* child = node->addChild(temp_board, opponent, is_push);
				m &= ~(1ull << bit);
			}

			if (!is_push) {
				for (std::vector<CNode*>::const_iterator i =
					node->getChildren().begin();
					i != node->getChildren().end(); i++) {
					CNode* child = *i;
					simulationPush(child);
				}
			}
		}
		else {
			CNode* child = node->addChild(board, player ^ 1, true);
		}
	}
}
#else
void simulationPush(CNode* node)
{
	const uint64_t* board = node->boardPointer();
	int player = node->player();

	uint64_t legalBoard = makeLegalBoard(board, player);

	if (legalBoard != 0ull) {
		const int opponent = player ^ 1;
		size_t legalBoardNum = std::bitset<64>(legalBoard).count();
		//bool is_push = (node->hierarchy() + 1) >= hierarchy_single || legalBoardNum == 1;
		bool is_push = (node->hierarchy() + 1) >= hierarchy_single;
		uint64_t m = legalBoard;
		int bit;
		while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
			uint64_t temp_board[2] = { board[0], board[1] };
			reverse(1ull << bit, temp_board, player);
			if (CNode* child = node->addChild(temp_board, opponent, is_push)) {
				if (!is_push) {
					simulationPush(child);
				}
			}
			m &= ~(1ull << bit);
		}

		//if (!is_push) {
		//    for (std::vector<CNode*>::const_iterator i =
		//        node->getChildren().begin();
		//        i != node->getChildren().end(); i++) {
		//        CNode* child = *i;
		//        simulationPush(child);
		//    }
		//}
	}
	else {
		player ^= 1;

		legalBoard = makeLegalBoard(board, player);

		if (legalBoard != 0ull) {
			const int opponent = player ^ 1;
			size_t legalBoardNum = std::bitset<64>(legalBoard).count();
			//bool is_push = (node->hierarchy() + 1) >= hierarchy_single || legalBoardNum == 1;
			bool is_push = (node->hierarchy() + 1) >= hierarchy_single;
			uint64_t m = legalBoard;
			int bit;
			while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
				uint64_t temp_board[2] = { board[0], board[1] };
				reverse(1ull << bit, temp_board, player);
				if (CNode* child = node->addChild(temp_board, opponent, is_push)) {
					if (!is_push) {
						simulationPush(child);
					}
				}
				m &= ~(1ull << bit);
			}

			//if (!is_push) {
			//	for (std::vector<CNode*>::const_iterator i =
			//		node->getChildren().begin();
			//		i != node->getChildren().end(); i++) {
			//		CNode* child = *i;
			//		simulationPush(child);
			//	}
			//}
		}
		else {
			CNode* child = node->addChild(board, player ^ 1, true);
		}
	}
}
#endif

CResult simulationSingle(const uint64_t board[], int player, const int hierarchy)
{
	CResult result;

	uint64_t legalBoard = makeLegalBoard(board, player);

	if (legalBoard != 0ull) {
		if (hierarchy <= hierarchy_cached) {
#if USE_SYMMETRY_OPTIMIZE
			for (int i = 0; i < 8; i++) {
				std::pair<uint64_t, uint64_t> b = { symmetry_naive(i, board[0]), symmetry_naive(i, board[1]) };
				auto it = result_cache[player][hierarchy].find(b);
				if (it != result_cache[player][hierarchy].end()) {
					CResult result(it->second);
#if DEBUG_PRINT
					final_num += result.match();
#endif
					return result;
				}
			}
#else
			auto it = result_cache[player][hierarchy].find(std::make_pair(board[0], board[1]));
			if (it != result_cache[player][hierarchy].end()) {
				CResult result(it->second);
				return result;
			}
#endif
		}

		const int opponent = player ^ 1;
		uint64_t m = legalBoard;
		int bit;
		while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
			uint64_t temp_board[2] = { board[0], board[1] };
			reverse(1ull << bit, temp_board, player);
			result.marge(simulationSingle(temp_board, opponent, hierarchy + 1));
			m &= ~(1ull << bit);
		}
	}
	else {
		player ^= 1;

		legalBoard = makeLegalBoard(board, player);

		if (legalBoard != 0ull) {
			if (hierarchy <= hierarchy_cached) {
#if USE_SYMMETRY_OPTIMIZE
				for (int i = 0; i < 8; i++) {
					std::pair<uint64_t, uint64_t> b = { symmetry_naive(i, board[0]), symmetry_naive(i, board[1]) };
					auto it = result_cache[player][hierarchy].find(b);
					if (it != result_cache[player][hierarchy].end()) {
						CResult result(it->second);
#if DEBUG_PRINT
						final_num += result.match();
#endif
						return result;
					}
				}
#else
				auto it = result_cache[player][hierarchy].find(std::make_pair(board[0], board[1]));
				if (it != result_cache[player][hierarchy].end()) {
					CResult result(it->second);
					return result;
				}
#endif
			}

			const int opponent = player ^ 1;
			uint64_t m = legalBoard;
			int bit;
			while ((bit = GetNumberOfTrailingZeros(m)) != 64) {
				uint64_t temp_board[2] = { board[0], board[1] };
				reverse(1ull << bit, temp_board, player);
				result.marge(simulationSingle(temp_board, opponent, hierarchy + 1));
				m &= ~(1ull << bit);
			}
		}
		else {
#if DEBUG_PRINT
			final_num++;
#endif
			result.set(board);
		}
	}

	if (hierarchy <= hierarchy_cached) {
		result_cache[player][hierarchy][{board[0], board[1]}] = result;
	}

	return result;
}

void reverse(const uint64_t put_mask, uint64_t board[], const int player)
{
	const int opponent = player ^ 1;

	// 着手した場合のボードを生成
	uint64_t rev = 0;
	for (int i = 0; i < 8; i++) {
		uint64_t rev_ = 0;
		uint64_t mask = transfer(put_mask, i);
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
