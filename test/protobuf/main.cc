#include "test.pb.h"
#include <iostream>
#include <string>
using namespace fixbug;
using namespace std;

int main(int argc, char const *argv[])
{
     GetFriendListsResponse rsp;
     ResultCode *rc = rsp.mutable_result();
     rc->set_errcode(0);
     
     User *user1 = rsp.add_friend_list();
     user1->set_name("zhang san");
     user1->set_age(20);
     user1->set_sex(User::MAN);

     User *user2 = rsp.add_friend_list();
     user2->set_name("zhang san");
     user2->set_age(20);
     user2->set_sex(User::MAN);

    cout << rsp.friend_list_size() << endl;
       
    return 0;
}


int main1() {
    LoginRequest req;
    req.set_name("zhang san");
    req.set_pwd("12345");

    string send_str;
    if (req.SerializeToString(&send_str)) {
        cout << send_str.c_str() << endl;
    }

    LoginRequest reqB;
    if (reqB.ParseFromString(send_str)) {
        cout << req.name() << endl;
        cout << req.pwd() << endl;
    }
    return 0;
}

