import torch

import Network

lr = 0.001
network, optimizer = Network.get_network("cpu", lr)
network.eval()

example = torch.randn(22, 20, 20, requires_grad=True)
print(example)

torch.onnx.export(network,
                  (example),
                  'model/model_latest.onnx',
                  input_names=['input'],
                  output_names=['value', "act"],
                  opset_version=17,
                  verbose=True)
