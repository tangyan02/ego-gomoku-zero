#include "../MCTS.h"

TEST_CASE("测试 add_dirichlet_noise 函数") {

    SUBCASE("基本功能测试") {
        std::vector<float> priors = {0.2f, 0.3f, 0.5f};
        std::vector<float> original_priors = priors;
        double epsilon = 0.25;
        double alpha = 1.0;

        std::mt19937 rng(42); // 固定随机种子

        MonteCarloTree tree(nullptr, 5, true);
        tree.add_dirichlet_noise(priors, epsilon, alpha, rng);

        // 检查向量大小不变
        CHECK(priors.size() == original_priors.size());

        // 检查值已改变
        bool values_changed = false;
        for (size_t i = 0; i < priors.size(); ++i) {
            if (priors[i] != original_priors[i]) {
                values_changed = true;
                break;
            }
        }
        CHECK(values_changed);

        // 检查归一化
        float sum = std::accumulate(priors.begin(), priors.end(), 0.0f);
        CHECK(sum == doctest::Approx(1.0f).epsilon(1e-5f));
    }

    SUBCASE("epsilon=0时不改变原值") {
        std::vector<float> priors = {0.2f, 0.3f, 0.5f};
        std::vector<float> original_priors = priors;
        std::mt19937 rng(42);

        MonteCarloTree tree(nullptr, 5, true);
        tree.add_dirichlet_noise(priors, 0.0, 1.0, rng);

        CHECK(priors == original_priors);
    }

    SUBCASE("空输入向量") {
        std::vector<float> priors;
        std::mt19937 rng(42);

        MonteCarloTree tree(nullptr, 5, true);
        tree.add_dirichlet_noise(priors, 0.25, 1.0, rng);

        CHECK(priors.empty());
    }
}