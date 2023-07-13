#include "mprpcchannel.h"
#include "string"
#include "rpcheader.pb.h"
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include "mprpcapplication.h"
#include <unistd.h>
#include "mprpccontroller.h"
#include "zookeeperutil.h"

// 客户调用远程方法
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                              google::protobuf::RpcController *controller, const google::protobuf::Message *request,
                              google::protobuf::Message *response, google::protobuf::Closure *done)
{
    const google::protobuf::ServiceDescriptor* sd = method->service();
    std::string service_name = sd->name();
    std::string method_name = method->name();

    uint32_t args_size = 0;
    std::string args_str;
    // 序列化
    if (request->SerializeToString(&args_str)) {
        args_size = args_str.size();
    }
    else {
        controller->SetFailed("serialize request error!");
        return;
    }

    // 设置头
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    // 头的长度
    uint32_t header_size = 0;
    // 头字符串
    std::string rpc_header_str;
    if (rpcHeader.SerializeToString(&rpc_header_str)) {
        header_size = rpc_header_str.size();
    }
    else {
        controller->SetFailed("serialize rpc header error!");
        return;
    }
    // 发送的string
    std::string send_rpc_str;
    send_rpc_str.insert(0, std::string((char*)&header_size, 4));
    send_rpc_str += rpc_header_str;
    send_rpc_str += args_str;

    std::cout << "======================================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl;
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_str: " << args_str << std::endl;
    std::cout << "======================================" << std::endl;

    // 创建socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1) {
        char errtxt[512] = {0};
        sprintf(errtxt, "create socket error! errno: %d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    // uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());

    ZkClient zkCli;
    zkCli.Start();
    std::string method_path = "/" + service_name + "/" + method_name;
    // 从zookeeper上获取服务ip port
    std::string host_data = zkCli.GetData(method_path.c_str());
    if (host_data == "") {
        controller->SetFailed(method_path + "is not exist!");
        return;
    }
    int idx = host_data.find(":");
    if (idx == -1) {
        controller->SetFailed(method_path + "address is invalid!");
        return;
    }
    // 获取的ip port
    std::string ip = host_data.substr(0, idx);
    uint16_t port = atoi(host_data.substr(idx + 1, host_data.size() - idx).c_str());

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    // 连接不成功
    if (connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "connect error! errno: %d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 发送不成功
    if (send(clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0) == -1) {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "send error! errno: %d", errno);
        controller->SetFailed(errtxt);
        return;
    }
    char recv_buf[1024] = {0};
    int recv_size = 0;
    // 接收不成功
    if ((recv_size = recv(clientfd, recv_buf, 1024, 0)) == -1) {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "recv error! errno: %d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 解析不成功
    if (!response->ParseFromArray(recv_buf, recv_size)) {
        close(clientfd);
        char errtxt[2048] = {0};
        sprintf(errtxt, "parse error! errno: %s", recv_buf);
        controller->SetFailed(errtxt);
        return;
    }
    // 关闭fd response里有响应
    close(clientfd);
}
