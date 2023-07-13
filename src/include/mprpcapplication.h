#pragma once
#include "mprpcconfig.h"

class MprpcApplication {
public: 
    static void Init(int argc, char** argv);
    static MprpcApplication& GetInstance();
    MprpcConfig& GetConfig();

private:
    static MprpcConfig m_config;    

    // 单例模式
    MprpcApplication(){};
    MprpcApplication(const MprpcApplication&) = delete;
    MprpcApplication(MprpcApplication&&) = delete;



};