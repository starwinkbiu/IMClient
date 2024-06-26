#ifndef NETMESSAGE_H
#define NETMESSAGE_H

/*
------------------------------------------
|  长度  |                 内容                   |
------------------------------------------
长度包含 1bit 处理模式和 7bit 内容长度
内容中前 2Bytes 为协议号剩下的内容为协议的附带内容
message_t:
type: 代表处理模式
size: 代表附带内容长度
protocol: 代表协议号
content: 为附带内容
*/

typedef struct message_t{
    message_t();
    ~message_t();
    bool type; // 标识二进制数据还是协议数据
    short size; // 消息的长度（协议+内容）
    short protocol; // 协议
    char* content; // 消息内容
    void initMessage();
    void setMessage(const char *_content, int _contentsize, bool _type, short _protocol);
} message_t;

#endif // NETMESSAGE_H
