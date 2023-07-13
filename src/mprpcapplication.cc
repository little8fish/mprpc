#include <iostream>
#include <string>
#include <unistd.h>
#include "mprpcapplication.h"
#include "mprpcconfig.h"


MprpcConfig MprpcApplication::m_config;

void ShowArgsHelp(){
    std::cout << "format: command -i <configfile>" << std::endl;
}

// 解析参数 加载配置文件
void MprpcApplication::Init(int argc, char** argv){
    if (argc < 2) {
        ShowArgsHelp();
        exit(EXIT_FAILURE);
    }
    int c = 0;
    std::string config_file;
    // 获取选项
    while ((c = getopt(argc, argv, "i:")) != -1) {
        switch (c)
        {
        case 'i':
            config_file = optarg;
            break;
        case '?':
            ShowArgsHelp();
            break;
        case ':':
            std::cout << "need <configfile>!" << std::endl;
            ShowArgsHelp();
            break;
        default:
            break;
        }
    }

    // 开始加载配置文件
    m_config.LoadConfigFile(config_file.c_str());

}

MprpcApplication& MprpcApplication::GetInstance()
{
    static MprpcApplication app;
    return app;
}

MprpcConfig& MprpcApplication::GetConfig() {
    return m_config;
}
