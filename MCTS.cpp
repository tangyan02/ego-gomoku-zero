#include "MCTS.h"

#include <random>
#include <numeric>
#include <cstring>
#include <chrono>
//#include <__random/random_device.h>

using namespace std;

Node::Node(Node *parent) : parent(parent), visits(0), value_sum(0), prior_prob(0), ucb(0), virtual_loss(0) {
}

bool Node::isLeaf() {
    return children.empty();
}

Node *Node::selectChild(double exploration_factor) {
    // 父节点的 effective visits 包含 virtual_loss
    double effective_parent_visits = static_cast<double>(visits) + static_cast<double>(virtual_loss);
    double sqrt_total = sqrt(effective_parent_visits);

    // FPU (First Play Urgency)：未访问子节点用父节点 Q 减去衰减值
    // 防止在优势局面下过度探索未访问节点
    // 视角说明：value_sum 累积的是"this 父视角下选 this 的预期收益"，
    //         而 fpu 估计的是 child 的 q（this 视角下选 child 的预期收益），
    //         视角差一次翻转，因此 baseline 取 -value_sum/visits。
    double parent_q = visits > 0 ? -value_sum / visits : 0.0;
    // 计算已访问子节点的 prior 总和，用于 FPU 衰减
    double visited_prior_sum = 0.0;
    for (const auto &[point, child] : children) {
        if (child->visits > 0) {
            visited_prior_sum += child->prior_prob;
        }
    }
    double fpu_value = parent_q - 0.2 * sqrt(visited_prior_sum);

    Node *selected = nullptr;
    double max_ucb = numeric_limits<double>::lowest();
    for (const auto &[point, child] : children) {
        // effective_visits 和 effective_value：把 virtual loss 当成 (visits++, value_sum-=1) 的悲观估计
        int effective_visits = child->visits + child->virtual_loss;
        double effective_value = child->value_sum - static_cast<double>(child->virtual_loss);
        // FPU：未访问节点用 fpu_value 代替 0
        double q = effective_visits > 0 ? effective_value / effective_visits : fpu_value;
        double ucb_value = q + exploration_factor * child->prior_prob *
                           sqrt_total / (1 + effective_visits);
        child->ucb = ucb_value;
        if (ucb_value > max_ucb) {
            max_ucb = ucb_value;
            selected = child;
        }
    }
    return selected;
}

void Node::addVirtualLoss() {
    virtual_loss++;
}

void Node::removeVirtualLoss() {
    virtual_loss--;
}

void Node::expand(Game &game, vector<Point> &moves, const vector<float> &probs_metrix) {
    //特殊处理跳过的情况
    vector<float> probs_arr;
    if (moves.size() > 1) {
        for (auto &move: moves) {
            int moveIndex = game.getActionIndex(move);
            probs_arr.emplace_back(probs_metrix[moveIndex]);
        }
    } else {
        probs_arr.emplace_back(1);
    }

    // 计算概率总和
    float sum_probs = 0.0;
    for (auto &prob: probs_arr) {
        sum_probs += prob;
    }

    for (int i = 0; i < moves.size(); i++) {
        auto move = moves[i];
        auto prob = probs_arr[i];
        Node *child = new Node(this);
        child->move = move;
        child->parent = this;

        // 归一化处理
        if (sum_probs != 0) {
            child->prior_prob = prob / sum_probs;
        } else {
            child->prior_prob = prob;
        }

        children[move] = child;
    }
}

void Node::update(double value) {
    visits++;
    value_sum += value;
}

MonteCarloTree::MonteCarloTree(Model *model, float exploration_factor, bool useNoice)
    : root(nullptr), model(model), exploration_factor(exploration_factor), useNoice(useNoice) {
}


void MonteCarloTree::simulate(Game game) {
    if (game.isGameOver()) {
        return;
    }

    Node *node = root;
    while (!node->isLeaf()) {
        auto result = node->selectChild(exploration_factor);
        game.makeMove(result->move);
        node = result;
    }

    float value;

    if (game.checkWin(game.lastAction.x, game.lastAction.y, game.getOtherPlayer())) {
        value = -1;
    } else {
        auto [win, moves, selectInfo] = selectActions(game);
        node->selectInfo = selectInfo;

        if (win) {
            // 必胜局面：跳过推理，value=+1
            value = 1;
            // 仅 root 第一次 expand（让 SelfPlay 能拿到 move）；非 root 永久 leaf
            if (node == root && node->isLeaf()) {
                // 用均匀 prior（不调推理）
                std::vector<float> uniform_priors(game.boardSize * game.boardSize, 0.0f);
                if (!moves.empty()) {
                    float p = 1.0f / moves.size();
                    for (auto& m : moves) {
                        uniform_priors[m.x * game.boardSize + m.y] = p;
                    }
                }
                node->expand(game, moves, uniform_priors);
            }
        } else {
            float eva_value;
            std::vector<float> probs_metrix;

            // Transposition Table 查询
            uint64_t hash = game.zobristHash;
            auto ttIt = transpositionTable.find(hash);
            if (ttIt != transpositionTable.end()) {
                // 命中缓存
                eva_value = ttIt->second.value;
                probs_metrix = ttIt->second.priors;
            } else {
                // 未命中，推理并缓存
                const int channels = INPUT_CHANNELS;
                float stateBuffer[channels * MAX_BOARD_SIZE * MAX_BOARD_SIZE];
                game.getState(stateBuffer, channels);
                auto result = model->evaluate_state(stateBuffer, channels, game.boardSize, game.boardSize);
                eva_value = result.first;
                probs_metrix = result.second;
                transpositionTable[hash] = TTEntry{eva_value, probs_metrix};
            }

            value = eva_value;
            if (useNoice && node == root) {
                add_dirichlet_noise(probs_metrix, moves, game.boardSize, 0.25, 0.2, rng);
            }
            node->expand(game, moves, probs_metrix);
        }
    }

    backpropagate(node, -value);
}

void MonteCarloTree::search(Game &game, Node *node, int num_simulations) {
    root = node;

    // 简单的模拟和更新逻辑
    for (int i = 0; i < num_simulations; i++) {
        //        cout << "开始模拟，次数 " << i << endl;
        simulate(game);
    }
}

// 走一条路径到叶子节点，沿途为每个访问的节点增加 virtual loss
// 返回叶子信息：如果叶子已经是终局/赢局，immediate_value 有值；否则需要加入 batch 推理
MonteCarloTree::LeafInfo MonteCarloTree::selectLeafWithVirtualLoss(Game game) {
    // 处理 game over 特殊情况（游戏已结束）：直接返回，无需操作
    if (game.isGameOver()) {
        return LeafInfo{nullptr, game, 0.0f, false};
    }

    Node *node = root;
    // 沿 PUCT 向下选择，直到叶子；沿途加 virtual loss
    while (!node->isLeaf()) {
        node->addVirtualLoss();
        Node *child = node->selectChild(exploration_factor);
        game.makeMove(child->move);
        node = child;
    }
    node->addVirtualLoss();

    // 判断叶子是否为终局：如果对方刚才的落子构成胜利，value = -1（当前玩家视角）
    if (game.lastAction.x >= 0 && game.lastAction.y >= 0 &&
        game.checkWin(game.lastAction.x, game.lastAction.y, game.getOtherPlayer())) {
        return LeafInfo{node, game, -1.0f, false};
    }

    // 需要评估的叶子
    return LeafInfo{node, game, 0.0f, true};
}

// 性能统计（累计）
static long long perf_selectLeaf_us = 0;
static long long perf_selectActions_us = 0;
static long long perf_getState_us = 0;
static long long perf_inference_us = 0;
static long long perf_expand_us = 0;
static long long perf_total_us = 0;
static int perf_call_count = 0;
static int perf_total_inferences = 0;

void MonteCarloTree::printPerfStats() {
    if (perf_call_count == 0) return;
    double total = perf_total_us / 1000.0;
    double sel = perf_selectLeaf_us / 1000.0;
    double sa = perf_selectActions_us / 1000.0;
    double gs = perf_getState_us / 1000.0;
    double inf = perf_inference_us / 1000.0;
    double exp = perf_expand_us / 1000.0;
    double other = total - sel - sa - gs - inf - exp;
    cout << "[Perf] " << perf_call_count << " steps | total " << (int)total << "ms"
         << " | selectLeaf " << (int)sel << "ms(" << (int)(sel/total*100) << "%)"
         << " | selectActions " << (int)sa << "ms(" << (int)(sa/total*100) << "%)"
         << " | getState " << (int)gs << "ms(" << (int)(gs/total*100) << "%)"
         << " | inference " << (int)inf << "ms(" << (int)(inf/total*100) << "%)"
         << " | expand " << (int)exp << "ms(" << (int)(exp/total*100) << "%)"
         << " | other " << (int)other << "ms(" << (int)(other/total*100) << "%)"
         << " | inferences=" << perf_total_inferences
         << endl;
    perf_selectLeaf_us = perf_selectActions_us = perf_getState_us = 0;
    perf_inference_us = perf_expand_us = perf_total_us = 0;
    perf_call_count = 0;
    perf_total_inferences = 0;
}

// 批量搜索：用 Virtual Loss + 批推理加速
void MonteCarloTree::searchBatched(Game &game, Node *node, int num_simulations, int batch_size) {
    root = node;

    // 如果游戏已结束，直接返回
    if (game.isGameOver()) {
        return;
    }

    auto perf_start = chrono::high_resolution_clock::now();
    int simulations_done = 0;
    const int channels = INPUT_CHANNELS;

    while (simulations_done < num_simulations) {
        int current_batch = std::min(batch_size, num_simulations - simulations_done);

        std::vector<LeafInfo> leaves;
        leaves.reserve(current_batch);

        // 1. 并行走多条路径到叶子，沿途加 virtual loss
        auto t_sel0 = chrono::high_resolution_clock::now();
        for (int i = 0; i < current_batch; i++) {
            leaves.push_back(selectLeafWithVirtualLoss(game));
        }
        auto t_sel1 = chrono::high_resolution_clock::now();
        perf_selectLeaf_us += chrono::duration_cast<chrono::microseconds>(t_sel1 - t_sel0).count();

        // 2. 收集需要网络评估的叶子：先调 selectActions 判 win，再查 TT
        std::vector<int> eval_indices;
        std::vector<int> tt_hit_indices;
        std::vector<int> win_indices;
        std::vector<bool> leaf_is_win(current_batch, false);
        std::vector<std::vector<Point>> leaf_moves(current_batch);
        std::vector<std::string> leaf_selectInfo(current_batch);
        std::vector<std::vector<float>> batch_states;
        eval_indices.reserve(current_batch);

        auto t_sa0 = chrono::high_resolution_clock::now();
        for (int i = 0; i < current_batch; i++) {
            if (leaves[i].leaf != nullptr && leaves[i].needs_eval) {
                auto [win, moves, selectInfo] = selectActions(leaves[i].game);
                leaf_is_win[i] = win;
                leaf_moves[i] = moves;
                leaf_selectInfo[i] = selectInfo;

                if (win) {
                    win_indices.push_back(i);
                    continue;
                }

                // 查 Transposition Table
                uint64_t hash = leaves[i].game.zobristHash;
                auto ttIt = transpositionTable.find(hash);
                if (ttIt != transpositionTable.end()) {
                    tt_hit_indices.push_back(i);
                } else {
                    eval_indices.push_back(i);
                }
            }
        }
        auto t_sa1 = chrono::high_resolution_clock::now();
        perf_selectActions_us += chrono::duration_cast<chrono::microseconds>(t_sa1 - t_sa0).count();

        // getState（TT 未命中的部分）
        auto t_gs0 = chrono::high_resolution_clock::now();
        for (int idx : eval_indices) {
            std::vector<float> state(channels * MAX_BOARD_SIZE * MAX_BOARD_SIZE);
            leaves[idx].game.getState(state.data(), channels);
            batch_states.push_back(std::move(state));
        }
        auto t_gs1 = chrono::high_resolution_clock::now();
        perf_getState_us += chrono::duration_cast<chrono::microseconds>(t_gs1 - t_gs0).count();

        // 3. 批量推理
        auto t_inf0 = chrono::high_resolution_clock::now();
        std::vector<std::pair<float, std::vector<float>>> batch_results;
        if (!batch_states.empty()) {
            int single_size = channels * MAX_BOARD_SIZE * MAX_BOARD_SIZE;
            std::vector<float> flat_batch(eval_indices.size() * single_size);
            for (size_t b = 0; b < batch_states.size(); b++) {
                std::memcpy(flat_batch.data() + b * single_size,
                           batch_states[b].data(), single_size * sizeof(float));
            }
            batch_results = model->evaluate_state_batch_flat(
                flat_batch.data(), eval_indices.size(), channels, MAX_BOARD_SIZE, MAX_BOARD_SIZE);
            perf_total_inferences += eval_indices.size();
        }
        auto t_inf1 = chrono::high_resolution_clock::now();
        perf_inference_us += chrono::duration_cast<chrono::microseconds>(t_inf1 - t_inf0).count();

        // 4. 处理 win 叶子（跳过推理，value=+1）
        for (int idx : win_indices) {
            Node *leaf = leaves[idx].leaf;
            leaf->selectInfo = leaf_selectInfo[idx];
            float value = 1.0f;

            // 仅 root leaf 第一次 expand 一次（让 SelfPlay 能拿到 move）；非 root 永久 leaf
            if (leaf == root && leaf->isLeaf()) {
                auto& moves = leaf_moves[idx];
                std::vector<float> uniform_priors(leaves[idx].game.boardSize * leaves[idx].game.boardSize, 0.0f);
                if (!moves.empty()) {
                    float p = 1.0f / moves.size();
                    for (auto& m : moves) {
                        uniform_priors[m.x * leaves[idx].game.boardSize + m.y] = p;
                    }
                }
                leaf->expand(leaves[idx].game, moves, uniform_priors);
            }

            // backpropagate + 移除 virtual loss
            Node *cur = leaf;
            float v = -value;
            while (cur != nullptr) {
                cur->removeVirtualLoss();
                cur->update(v);
                cur = cur->parent;
                v = -v;
            }
        }

        // 5. 处理 TT 命中的叶子（非 win）
        for (int idx : tt_hit_indices) {
            Node *leaf = leaves[idx].leaf;
            uint64_t hash = leaves[idx].game.zobristHash;
            auto &ttEntry = transpositionTable[hash];

            leaf->selectInfo = leaf_selectInfo[idx];
            float value = ttEntry.value;
            auto probs_copy = ttEntry.priors;
            if (useNoice && leaf == root) {
                add_dirichlet_noise(probs_copy, leaf_moves[idx], leaves[idx].game.boardSize, 0.25, 0.2, rng);
            }
            leaf->expand(leaves[idx].game, leaf_moves[idx], probs_copy);

            // backpropagate + 移除 virtual loss
            Node *cur = leaf;
            float v = -value;
            while (cur != nullptr) {
                cur->removeVirtualLoss();
                cur->update(v);
                cur = cur->parent;
                v = -v;
            }
        }

        // 5. 处理网络推理结果
        int eval_ptr = 0;
        for (int idx : eval_indices) {
            Node *leaf = leaves[idx].leaf;

            auto &[eva_value, probs_metrix] = batch_results[eval_ptr++];

            // 写入 TT 缓存
            uint64_t hash = leaves[idx].game.zobristHash;
            transpositionTable[hash] = TTEntry{eva_value, probs_metrix};

            // selectInfo / moves 已在前面 selectActions 时拿到（保证 win=false，因为 win 已在前面处理）
            leaf->selectInfo = leaf_selectInfo[idx];
            float value = eva_value;
            auto probs_copy = probs_metrix;
            if (useNoice && leaf == root) {
                add_dirichlet_noise(probs_copy, leaf_moves[idx], leaves[idx].game.boardSize, 0.25, 0.2, rng);
            }
            leaf->expand(leaves[idx].game, leaf_moves[idx], probs_copy);

            // backpropagate + 移除 virtual loss
            Node *cur = leaf;
            float v = -value;
            while (cur != nullptr) {
                cur->removeVirtualLoss();
                cur->update(v);
                cur = cur->parent;
                v = -v;
            }
        }

        // 6. 处理终局叶子（needs_eval=false 且非 TT 命中的）
        for (int i = 0; i < current_batch; i++) {
            Node *leaf = leaves[i].leaf;
            if (leaf == nullptr) continue;
            if (leaves[i].needs_eval) continue;  // 已在上面处理

            float value = leaves[i].immediate_value;
            Node *cur = leaf;
            float v = -value;
            while (cur != nullptr) {
                cur->removeVirtualLoss();
                cur->update(v);
                cur = cur->parent;
                v = -v;
            }
        }

        simulations_done += current_batch;
    }

    auto perf_end = chrono::high_resolution_clock::now();
    perf_total_us += chrono::duration_cast<chrono::microseconds>(perf_end - perf_start).count();
    perf_call_count++;
}

void MonteCarloTree::backpropagate(Node *node, float value) {
    while (node != nullptr) {
        node->update(value);
        node = node->parent;
        value = -value;
    }
}

pair<vector<Point>, vector<float> > MonteCarloTree::get_action_probabilities(float temperature) {

    Node *node = root;
    vector<pair<Point, int> > action_visits;
    for (auto &item: node->children) {
        action_visits.emplace_back(item.first, item.second->visits);
    }

    vector<Point> moves;
    vector<int> visits;
    for (auto &item: action_visits) {
        moves.push_back(item.first);
        visits.push_back(item.second);
    }

    vector<float> action_probs(moves.size(), 0.0f);

    // 防止温度过小导致溢出
    if (temperature < 1e-3) {
        // 只选最大访问数的动作
        int max_visit = -1;
        size_t max_idx = 0;
        for (size_t i = 0; i < visits.size(); ++i) {
            if (visits[i] > max_visit) {
                max_visit = visits[i];
                max_idx = i;
            }
        }
        if (!visits.empty()) {
            action_probs[max_idx] = 1.0f;
        }
    } else {
        // 用log技巧防止溢出
        std::vector<float> log_probs(moves.size(), 0.0f);
        float max_log = -std::numeric_limits<float>::infinity();
        for (size_t i = 0; i < visits.size(); ++i) {
            if (visits[i] > 0) {
                log_probs[i] = std::log(static_cast<float>(visits[i])) / temperature;
                if (log_probs[i] > max_log) max_log = log_probs[i];
            } else {
                log_probs[i] = -std::numeric_limits<float>::infinity();
            }
        }
        // log-sum-exp
        float sum_exp = 0.0f;
        for (size_t i = 0; i < log_probs.size(); ++i) {
            if (log_probs[i] > -std::numeric_limits<float>::infinity()) {
                action_probs[i] = std::exp(log_probs[i] - max_log);
                sum_exp += action_probs[i];
            } else {
                action_probs[i] = 0.0f;
            }
        }
        if (sum_exp > 0.0f) {
            for (auto &prob: action_probs) {
                prob /= sum_exp;
            }
        }
    }

    return make_pair(moves, action_probs);
}


pair<vector<Point>, vector<float> > MonteCarloTree::get_action_probabilities() {
    Node *node = root;
    vector<pair<Point, int> > action_visits;
    for (auto &item: node->children) {
        action_visits.emplace_back(item.first, item.second->visits);
    }

    vector<Point> moves;
    vector<int> visits;
    for (auto &item: action_visits) {
        moves.push_back(item.first);
        visits.push_back(item.second);
    }

    // 计算总和
    int sum = 0;
    for (int visit: visits) {
        sum += visit;
    }

    // 归一化为概率分布
    vector<float> action_probs;
    for (const int visit: visits) {
        float prob = static_cast<float>(visit) / static_cast<float>(sum);
        action_probs.push_back(prob);
    }

    return make_pair(moves, action_probs);
}

Point MonteCarloTree::get_max_visit_move() {
    Node* node = root;
    vector<pair<Point, int>> action_visits;

    Point maxVisitMove;
    int maxVisit = -1;
    for (auto& item : node->children)
    {
        auto visit = item.second->visits;
        if (visit > maxVisit)
        {
            maxVisit = visit;
            maxVisitMove = item.first;
        }
    }
    return maxVisitMove;
}

void Node::release() {
    for (const auto &item: this->children) {
        item.second->release();
    }
    if (this->parent != nullptr) {
        delete this;
    }
}
std::vector<double> MonteCarloTree::sample_dirichlet(int size, double alpha, std::mt19937 &rng) {
    std::gamma_distribution gamma_dist(alpha, 1.0);
    std::vector<double> samples(size);
    double sum = 0.0;
    for (int i = 0; i < size; ++i) {
        samples[i] = gamma_dist(rng);
        sum += samples[i];
    }
    // 归一化
    for (int i = 0; i < size; ++i) {
        samples[i] /= sum;
    }
    return samples;
}

void MonteCarloTree::add_dirichlet_noise(std::vector<float> &priors, const std::vector<Point> &moves,
                                         int boardSize, double epsilon, double alpha, std::mt19937 &rng) {
    // 只对合法落子点采样 Dirichlet 噪声。
    // 若对整个 boardSize*boardSize 维加噪，合法点分到的噪声会被非法点稀释到近乎为 0，探索失效。
    int n = (int)moves.size();
    if (n == 0) return;
    std::vector<double> noise = sample_dirichlet(n, alpha, rng);
    double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        int idx = moves[i].x * boardSize + moves[i].y;
        priors[idx] = float((1 - epsilon) * priors[idx] + epsilon * noise[i]);
        sum += priors[idx];
    }
    // 在合法点集合内归一化（非法点本就为 0，不参与）
    if (sum > 0.0) {
        for (int i = 0; i < n; ++i) {
            int idx = moves[i].x * boardSize + moves[i].y;
            priors[idx] /= float(sum);
        }
    }
}

// 紧凑版：priors 每个元素都对应一个合法点，直接整体加噪
void MonteCarloTree::add_dirichlet_noise(std::vector<float> &priors, double epsilon, double alpha, std::mt19937 &rng) {
    int size = priors.size();
    if (size == 0) return;
    std::vector<double> noise = sample_dirichlet(size, alpha, rng);
    double sum = 0.0;
    for (int i = 0; i < size; ++i) {
        priors[i] = float((1 - epsilon) * priors[i] + epsilon * noise[i]);
        sum += priors[i];
    }
    if (sum > 0.0) {
        for (int i = 0; i < size; ++i) {
            priors[i] /= float(sum);
        }
    }
}