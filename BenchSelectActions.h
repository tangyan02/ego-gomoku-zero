#ifndef EGO_GOMOKU_ZERO_BENCH_SELECT_ACTIONS_H
#define EGO_GOMOKU_ZERO_BENCH_SELECT_ACTIONS_H

// 性能基准：对每个开局局面，对比 selectActions 基线、加 5 层 VCT、加 7 层 VCT 的耗时与必胜发现率

void runBenchSelectActions();

#endif //EGO_GOMOKU_ZERO_BENCH_SELECT_ACTIONS_H
