#include <string.h>
#include "netMessage.h"

message_t::message_t():type(false), size(0), protocol(0), content(NULL){}

message_t::~message_t()
{
    initMessage();
}

void message_t::initMessage()
{
    type = false;
    if(content)
        delete[] content;
    size = 0;
    protocol = 0;
}

void message_t::setMessage(const char *_content, int _contentsize, bool _type, short _protocol)
{
    content = new char[_contentsize + 1];
    type = _type;
    protocol = _protocol;
    size = _contentsize + sizeof(protocol);
    // 复制内容
    memcpy(content, _content, _contentsize);
}
