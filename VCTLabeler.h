#ifndef EGO_GOMOKU_ZERO_VCTLABELER_H
#define EGO_GOMOKU_ZERO_VCTLABELER_H

// 离线 VCT 标注：遍历 record/*.txt，对每个样本从当前玩家视角判断是否存在必胜
// （selectActions 快速路径 / dfsVCTIter 深度路径），若是则将 value 覆盖为 +1.0。
//
// 配置：
//   vctLabelMaxTimeMs  : 单个样本 VCT 搜索超时（默认 5000ms）
//   vctLabelProcesses  : 并行线程数（默认 8）
//   vctLabelRecordDir  : record 目录（默认 "record"）
//
// 运行方式：Main.cpp 设置 mode=vct_label 时调用

void runVCTLabeling();

#endif //EGO_GOMOKU_ZERO_VCTLABELER_H
