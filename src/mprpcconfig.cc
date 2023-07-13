#include <iostream>
#include <string>
#include "mprpcconfig.h"

// 对一个字符串去掉前后空格
void MprpcConfig::Trim(std::string &src_buf){
    // 去掉字符串前面的空格
    int idx = src_buf.find_first_not_of(' ');
    if (idx != -1) {
        src_buf = src_buf.substr(idx, src_buf.size() - idx);
    }
    // 去掉字符串后面的空格
    idx = src_buf.find_last_not_of(' ');
    if (idx != -1) {
        src_buf = src_buf.substr(0, idx + 1);
    }
}

// 加载配置文件信息 以键值对存储到map里
void MprpcConfig::LoadConfigFile(const char *config_file)
{
    // 只读方式打开文件
    FILE *pf = fopen(config_file, "r");
    if (pf == nullptr) {
        std::cout << config_file << " is not exist!" << std::endl;
        exit(EXIT_FAILURE);
    }
    // 当没有到文件尾部时
    while (!feof(pf)) {
        char buf[512] = {0};
        // fgets最多只能读取一行数据
        fgets(buf, 512, pf);

        // 去除空格
        std::string read_buf(buf);
        Trim(read_buf);

        // 跳过注释行
        if (read_buf[0] == '#' || read_buf.empty()) {
            continue;
        }
        // 查找 = 
        int idx = read_buf.find('=');
        if (idx == -1) {
            continue;
        }
        std::string key;
        std::string value;
        key = read_buf.substr(0, idx);
        Trim(key);
        // 找到换行符
        int endidx = read_buf.find('\n', idx + 1);
        value = read_buf.substr(idx + 1, endidx - idx - 1);
        Trim(value);
        // 插入 key value
        m_configMap.insert({key, value});
    }
}

// 获取信息
std::string MprpcConfig::Load(const std::string &key)
{
    auto it = m_configMap.find(key);
    if (it == m_configMap.end()) {
        return "";
    }
    return it->second;
}