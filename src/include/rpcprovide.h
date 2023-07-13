#pragma once
#include "google/protobuf/service.h"
#include <memory>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <string>

class RpcProvider
{
public:
    void NotifyService(google::protobuf::Service *service);

    void Run();

private:
    muduo::net::EventLoop m_eventLoop;

    // 服务 各种方法
    struct ServiceInfo
    {
        google::protobuf::Service *m_service;
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor*> m_methodMap;
    };

    // 存储服务
    std::unordered_map<std::string, ServiceInfo> m_serviceMap;

    // 连接处理
    void OnConnection(const muduo::net::TcpConnectionPtr &);
    // 消息处理
    void OnMessage(const muduo::net::TcpConnectionPtr &, muduo::net::Buffer *, muduo::Timestamp);
    // Closure的回调操作 用于序列化rpc的响应和发送
    void SendRpcResponse(const muduo::net::TcpConnectionPtr&, google::protobuf::Message*);
};