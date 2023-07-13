#include <iostream>
#include <string>
#include "friend.pb.h"
#include "mprpcapplication.h"
#include "rpcprovide.h"
#include "logger.h"
#include <vector>

class FriendService : public fixbug::FriendServiceRpc
{
public:
    // 本地函数
    std::vector<std::string> GetFriendsList(uint32_t userid)
    {
        std::cout << "do GetFriendsList service! userid: " << userid << std::endl;
        std::vector<std::string> vec;

        vec.push_back("gao yang");
        vec.push_back("liu hong");
        vec.push_back("wang shuo");
        return vec;
    }
    // rpc函数
    void GetFriendsList(google::protobuf::RpcController *controller,
                        const ::fixbug::GetFriendsListRequest *request,
                        ::fixbug::GetFriendsListResponse *response,
                        ::google::protobuf::Closure *done)
    {
        // 获取参数
        uint32_t userid = request->userid();
        // 调用本地函数
        std::vector<std::string> friendsList = GetFriendsList(userid);
        // 封装结果
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        for (std::string &name: friendsList) {
            std::string *p = response->add_friends();
            *p = name;
        }
        // closure->SendRpcResponse 就是发送rpc相应给客户
        done->Run();
    }
};

int main(int argc, char *argv[])
{
    // 流式消息
    LOG_INFO("first log message!");
    // LOG_ERR("%s:%s:%d", __FILE__, __FUNCTION__, __LINE__);

    // 加载配置文件 保存到m_config中
    MprpcApplication::Init(argc, argv);

    // provider 是一个rpc网络服务对象 把FriendService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new FriendService());

    // 启动一个rpc服务发布节点 Run以后 进程进入阻塞状态 等待远程的rpc调用请求
    // 当接收到Message后 调用provider.OnMessage 
    provider.Run();


    return 0;
}