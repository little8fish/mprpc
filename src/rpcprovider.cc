#include "rpcprovide.h"
#include "mprpcapplication.h"
#include "google/protobuf/descriptor.h"
#include "rpcheader.pb.h"
#include "zookeeperutil.cc"

// 注册服务
void RpcProvider::NotifyService(google::protobuf::Service *service)
{
    ServiceInfo service_info;
    auto pserviceDesc = service->GetDescriptor();
    // service name
    std::string service_name = pserviceDesc->name();
    std::cout << "service_name: " << service_name << std::endl;
    // 方法数量
    int methodCnt = pserviceDesc->method_count();
    for (int i = 0; i < methodCnt; ++i)
    {
        auto pmethodDesc = pserviceDesc->method(i);
        // method name
        std::string method_name = pmethodDesc->name();
        std::cout << "method_name: " << method_name << std::endl;
        // 存储method name
        service_info.m_methodMap.insert({method_name, pmethodDesc});
    }
    service_info.m_service = service;
    // 存储服务 
    m_serviceMap.insert({service_name, service_info});
}

// 处理连接
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        conn->shutdown();
    }
}


// 主要在这个函数里 负责处理
// 收到消息 解析 service_name method_name args 找到service method 构造 request response 远程调用（绑定了响应函数，响应直接发送给客户端）
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buffer, muduo::Timestamp time)
{
    std::string recv_buf = buffer->retrieveAllAsString();
    uint32_t header_size = 0;
    // 前4字节放到header_size
    recv_buf.copy((char *)&header_size, 4, 0);
    // 取出头
    std::string rpc_header_str = recv_buf.substr(4, header_size);
    // protobuf生成的rpcHeader类
    mprpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    // 解析头 其实是利用了封装和解析
    if (rpcHeader.ParseFromString(rpc_header_str))
    {
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();
    }
    else
    {
        std::cout << "rpc_header_str:" << rpc_header_str << " parse error!" << std::endl;
        return;
    }
    // 获取参数字符串
    std::string args_str = recv_buf.substr(4 + header_size, args_size);

    std::cout << "======================================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl;
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_str: " << args_str << std::endl;
    std::cout << "======================================" << std::endl;

    // 找到服务类
    auto it = m_serviceMap.find(service_name);
    if (it == m_serviceMap.end())
    {
        std::cout << service_name << " is not exist!" << std::endl;
        return;
    }

    // 找到服务方法
    auto mit = it->second.m_methodMap.find(method_name);
    if (mit == it->second.m_methodMap.end())
    {
        std::cout << service_name << ":" << method_name << " is not exist!" << std::endl;
        return;
    }

    // 服务类 和 方法
    google::protobuf::Service *service = it->second.m_service;
    const google::protobuf::MethodDescriptor *method = mit->second;

    // 封装请求
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(args_str))
    {
        std::cout << "request parse error, content: " << args_str << std::endl;
        return;
    }
    // 封装响应
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    // 给done绑定SendRpcResponse方法
    google::protobuf::Closure *done = google::protobuf::NewCallback<RpcProvider,
                                  const muduo::net::TcpConnectionPtr &,
                                  google::protobuf::Message *>(this, &RpcProvider::SendRpcResponse, conn, response);
    // 感觉还是由框架发起的service的方法的调用
    // 会转到service 调用对应方法 并且done绑定了SendRpcResponse方法 最终会发送响应给客户
    service->CallMethod(method, nullptr, request, response, done);
}

// 序列化rpc响应 发送rpc响应
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response)
{
    std::string response_str;
    if (response->SerializeToString(&response_str)) {
        conn->send(response_str);
    }
    else {
        std::cout << "serialize response_str error!" << std::endl;
    }
    conn->shutdown();
}

void RpcProvider::Run()
{
    // 获取ip和port
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    muduo::net::InetAddress address(ip, port);

    // 初始化server
    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");
    // 绑定连接回调和消息回调
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    server.setThreadNum(4);

    // zookeeper客户
    ZkClient zkCli;
    zkCli.Start();

    // 依据服务创建zookeeper节点
    for (auto &sp: m_serviceMap) {
        std::string service_path = "/" + sp.first;
        zkCli.Create(service_path.c_str(), nullptr, 0);
        for (auto &mp: sp.second.m_methodMap) {
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            // ZOO_EPHEMERAL表示临时节点
            zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }

    std::cout << "RpcProvider start service at ip:" << ip << " port:" << port << std::endl;

    server.start();
    // 为什么这里要开loop
    m_eventLoop.loop();
}