from torch.utils.data import Dataset


class SampleSet(Dataset):
    def __init__(self, data):
        self.data = data

    def __len__(self):
        return len(self.data)

    def __getitem__(self, index):
        sample = self.data[index]
        # 在这里可以对样本进行预处理、转换等操作
        return sample
