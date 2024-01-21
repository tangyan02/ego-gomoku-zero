from torch.utils.data import Dataset


class ReplayBuffer(Dataset):
    def __init__(self, max_size=None):
        self.training_data = []
        self.max_size = max_size

    def add_samples(self, new_samples):
        self.training_data.extend(new_samples)
        if self.max_size is not None and len(self.training_data) > self.max_size:
            self.training_data = self.training_data[-self.max_size:]

    def size(self):
        return len(self.training_data)

    def __getitem__(self, index):
        return self.training_data[index]

    def __len__(self):
        return len(self.training_data)
