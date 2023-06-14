# This is a sample Python script.
import os
import argparse
import xml.sax
import traceback
from logs import logger

software_version = "v0.1.2"


def SoftWareVersion():
    return software_version


def BinaryUint(length):
    uint = ["B", "KB", "MB", "GB", "TB", "PB"]
    size = 1024
    for it in range(len(uint)):
        if (length / size) < 1:
            return "%.2f%s" % (length, uint[it])
        length = length / size


class BinFile:
    def __init__(self, path, offset, length):
        self.path = path
        self.offset = offset
        self.length = length


class XmlHandler(xml.sax.ContentHandler):
    def __init__(self):
        self.current = ""
        self.file = ""
        self.offset = 0x00000000
        self.length = 0x00000000
        self.list = []

    def startElement(self, tag, attributes):
        self.current = tag
        # 解析bin文件名称
        if tag == "bin":
            self.file = attributes["name"]

    def endElement(self, tag):
        # 解析元素结束，将对象推送至列表
        if tag == 'bin':
            binary = BinFile(self.file, self.offset, self.length)
            self.list.append(binary)
            self.file = ""
            self.offset = 0x00000000
            self.length = 0x00000000
        # 解析元素完成，设置缓冲数据为空
        self.current = ""

    def characters(self, content):
        # 解析bin文件的偏移地址
        if self.current == "offset":
            self.offset = content
        # 解析bin文件的长度信息
        elif self.current == "length":
            self.length = content

    def result(self):
        return self.list

    def clear(self):
        return self.list.clear()


class ParseConfigFile:
    xml_parser = None
    xml_handle = None

    def __init__(self, path):
        if (path is not None) and os.path.exists(path):
            try:
                # 初始化xml解析对象
                self.xml_handle = XmlHandler()
                self.xml_parser = xml.sax.make_parser()
                self.xml_parser.setFeature(xml.sax.handler.feature_namespaces, 0)
                self.xml_parser.setContentHandler(self.xml_handle)
                # 解析配置文件
                self.xml_parser.parse(path)
            except Exception as ex:
                logger.error(ex.__str__())
                logger.error(traceback.format_exc())
                logger.error("des : binary config file {} is parse error.".format(path))
                self.xml_handle.clear()
        else:
            logger.warning("des : binary config file does not exist.")

    def process(self):
        return self.xml_handle.result()


def BinaryProcess(input_file, output_file, config_file):
    # 初始化xml解析对象
    xml_parser = xml.sax.make_parser()
    xml_parser.setFeature(xml.sax.handler.feature_namespaces, 0)
    xml_parser.setContentHandler(XmlHandler())
    # 解析flash相关配置文件
    bin_list = ParseConfigFile(config_file).process()
    # 打开输入文件
    input_bin = open(input_file, 'rb')
    for it in bin_list:
        # 如果输入为文件，则获取文件路径
        if os.path.isfile(output_file):
            output_dir = os.path.dirname(output_file)
        else:
            output_dir = output_file
        # 如果给路径不存在，则创建路径
        if os.path.exists(output_dir) is False:
            os.makedirs(output_dir)
        # 拼接得到文件的输出路径
        output_path = os.path.join(output_dir, it.path)
        # 如果输出路径中存在同名文件，则删除原来的同名文件
        if os.path.exists(output_path) is True:
            os.remove(output_path)
        # 打开输出文件
        output_bin = open(output_path, 'wb+')
        # 输入文件偏移到指定地址读取指定长度的数据后关闭文件
        output_offset = int(it.offset, 16)
        output_length = int(it.length, 16)
        input_bin.seek(output_offset, 0)
        input_data = input_bin.read(output_length)
        # 读取的数据写入到输出文件后关闭文件
        output_bin.write(input_data)
        output_bin.close()
        # 调试信息
        logger.info("====================================================")
        logger.info("binary output = {}".format(output_path))
        logger.info("binary length = {}".format(BinaryUint(output_length)))
        logger.info("====================================================")
    # 关闭输入文件
    input_bin.close()


def main():
    parser = argparse.ArgumentParser(description='Binary file process tools')

    parser.add_argument('-i', '--input',
                        help='Binary file path.',
                        dest="input",
                        required=True)
    parser.add_argument('-o', '--output',
                        help='Output file path.',
                        dest="output",
                        required=True)
    parser.add_argument('-c', '--config',
                        help='Config file path.',
                        dest="config",
                        required=True)
    parser.add_argument("-v", "--version",
                        help="Show the current version.",
                        action="version",
                        version=SoftWareVersion())
    parser.add_argument("-l", "--log",
                        help="Log level:10-debug,20-info,30-warning,40-error,50-critical.",
                        dest="log",
                        default=10,
                        required=False)

    args = parser.parse_args()
    # 重新设置日志输出级别：10-debug, 20-info, 30-warning, 40-error, 50-critical
    logger.set_level(int(args.log))
    # 输出参数调试信息
    logger.info("Binary Inputs path = {}".format(args.input))
    logger.info("Binary Output Path = {}".format(args.output))
    logger.info("Binary Config Path = {}".format(args.config))
    # 待处理文件
    input_path = os.path.abspath(args.input)
    # 输出文件
    output_path = os.path.abspath(args.output)
    # 配置文件
    config_path = os.path.abspath(args.config)
    # 执行处理
    BinaryProcess(input_path, output_path, config_path)


# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    main()
