import logging
import sys


class Logs:
    def __init__(self, name):
        self.logger = logging.getLogger(name)
        self.logger.setLevel(level=logging.INFO)
        self.formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        # 文件log
        # self.fHandler = logging.FileHandler("image_factory_log.txt", mode='a')
        # self.fHandler.setLevel(level=logging.INFO)
        # self.fHandler.setFormatter(self.formatter)
        # self.logger.addHandler(self.fHandler)
        # 控制台log
        self.console = logging.StreamHandler(stream=sys.stdout)
        self.console.setLevel(level=logging.INFO)
        self.console.setFormatter(self.formatter)
        self.logger.addHandler(self.console)

    def debug(self, content):
        self.logger.debug(content)

    def info(self, content):
        self.logger.info(content)

    def warning(self, content):
        self.logger.warning(content)

    def error(self, content):
        self.logger.error(content)

    def set_level(self, level):
        self.logger.setLevel(level=level)
        for handler in self.logger.handlers:
            handler.setLevel(level=level)


# 初始化日志对象
logger = Logs(__name__)