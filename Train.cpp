#include "Train.h"

void
train(ReplayBuffer &replay_buffer, const std::shared_ptr<PolicyValueNetwork> &network, Device device, float lr,
      int num_epochs, int batch_size) {
    // 创建数据集
    auto dataloader = torch::data::make_data_loader<torch::data::samplers::RandomSampler>(std::move(replay_buffer),
                                                                                          batch_size);

    // 定义损失函数
    auto criterion = MSELoss();
    // 定义优化器
    auto optimizer = Adam(network->parameters(), AdamOptions(lr));
    // 训练循环
    for (int epoch = 0; epoch < num_epochs; ++epoch) {
        float running_loss = 0.0;
        float running_count = 0.0;
        for (auto &batch_data: *dataloader) {
            vector<Tensor> stateList;
            vector<Tensor> mctsProbTensorList;
            vector<Tensor> valueTensorList;
            for (const auto &item: batch_data) {
                stateList.emplace_back(get<0>(item));

                vector<float> mctsProbList = get<1>(item);
                torch::Tensor mctsProbTensor = torch::from_blob(mctsProbList.data(),
                                                                {static_cast<long>(mctsProbList.size())});
                mctsProbTensorList.emplace_back(mctsProbTensor.clone());

                vector<float> valueList = get<2>(item);
                torch::Tensor valueTensor = torch::from_blob(valueList.data(), {static_cast<long>(valueList.size())});
                valueTensorList.emplace_back(valueTensor.clone());
            }
            Tensor states = torch::stack(stateList, 0).to(device);
            Tensor mcts_probs = torch::stack(mctsProbTensorList, 0).to(device);
            Tensor values = torch::stack(valueTensorList, 0).to(device);

            optimizer.zero_grad();

            // 前向传播
            auto predicted_data = network->forward(states);
            auto prodicted_value = get<0>(predicted_data);
            auto prodicted_act = get<1>(predicted_data);

            // 计算值和策略的损失
            auto value_loss = criterion(prodicted_value, values);

            // 计算交叉熵损失
            auto policy_loss = -torch::mean(torch::sum(mcts_probs * prodicted_act, 1));

            // 总损失
            auto loss = value_loss + policy_loss;

            // 反向传播和优化
            loss.backward();
            optimizer.step();

            running_loss += loss.item<float>();
            running_count += 1;
            break;
        }

        std::cout << "Epoch " << epoch + 1 << "/" << num_epochs << ", Loss: "
                  << running_loss / running_count << std::endl;
    }
}