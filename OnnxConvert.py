import onnx
import torch
from onnxsim import simplify

import Network

lr = 0.001
network, optimizer = Network.get_network("cpu", lr)
network.eval()

path = 'model/model_latest.onnx'

example = torch.randn(22, 20, 20, requires_grad=True)
print(example)

torch.onnx.export(network,
                  (example),
                  path,
                  input_names=['input'],
                  output_names=['value', "act"],
                  opset_version=17,
                  verbose=True)

onnx_model = onnx.load(path)  # load onnx model
model_simp, check = simplify(onnx_model)
assert check, "Simplified ONNX model could not be validated"
onnx.save(model_simp, path + "_simple")
print('finished exporting onnx')
