class ConfigReader:

    config = {}

    @staticmethod
    def init():
        file_path = "application.conf"
        with open(file_path, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#'):  # 忽略注释和空行
                    key, value = line.split('=', 1)
                    ConfigReader.config[key.strip()] = value.strip()

    @staticmethod
    def get(key):
        return ConfigReader.config[key.strip()]