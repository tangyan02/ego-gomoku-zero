#include "Train.h"

void train(ReplayBuffer& replay_buffer, const std::__1::shared_ptr<PolicyValueNetwork>& network, Device device, float lr, int num_epochs, int batch_size) {
    // 创建数据加载器
    auto dataloader = torch::data:: DataLoader(replay_buffer, batch_size, true);
    // 定义损失函数
    auto criterion = MSELoss();
    // 定义优化器
    auto optimizer = Adam(network->parameters(), AdamOptions(lr));
    // 训练循环
    for (int epoch = 0; epoch < num_epochs; ++epoch) {
        float running_loss = 0.0;
        for (auto& batch_data : *dataloader) {
            auto states = batch_data.data.to(device);
            auto mcts_probs = batch_data.target.to(device);
            auto values = batch_data.target2.to(device);

            optimizer.zero_grad();

            // 前向传播
            auto predicted_values = network->forward(states);

            // 计算值和策略的损失
            auto value_loss = criterion(predicted_values, values);

            // 计算交叉熵损失
            auto policy_loss = -torch::mean(torch::sum(mcts_probs * predicted_values, 1));

            // 总损失
            auto loss = value_loss + policy_loss;

            // 反向传播和优化
            loss.backward();
            optimizer.step();

            running_loss += loss.item<float>();
        }

        std::cout << "Epoch " << epoch + 1 << "/" << num_epochs << ", Loss: " << running_loss / dataloader->size() << std::endl;
    }
}