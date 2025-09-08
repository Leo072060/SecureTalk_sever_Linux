#include <iostream>
#include <stdlib.h>
#include <string>
#include <unistd.h>

#include "logManager.h"
#include "msgHandler.h"
#include "networkManager.h"
#include <hiredis/hiredis.h>

int TEST();
int main()
{
    // Initialize loggers
    InitLogger();
    LOG_INFO(logger, "Hello, this is SecureTalk server!");

    // Start redis
    if (system("redis-server --daemonize yes") != 0)
    {
        LOG_ERROR(logger, "Failed to start redis-server");
        throw std::runtime_error("Failed to start redis-server");
    }
    usleep(500000); // wait for redis to start
    LOG_INFO(logger, "Redis started");

    // TEST
    TEST();

    // Register message handlers
    NetworkManager::instance()->addMessageHandler(MsgType::LOGIN_REQUEST, HandleLoginRequest);
    NetworkManager::instance()->addMessageHandler(MsgType::SIGN_UP_REQUEST, HandleSignUpRequest);

    // Start the network manager
    NetworkManager::instance()->start(7777);

    // Stop redis
    // system("redis-cli shutdown");

    LOG_INFO(logger, "Server stopped");
    return 0;
}

int TEST()
{
    // 连接到 Redis 服务器（默认 127.0.0.1:6379）
    redisContext *c = redisConnect("127.0.0.1", 6379);
    if (c == nullptr || c->err)
    {
        if (c)
        {
            std::cerr << "Connection error: " << c->errstr << std::endl;
            redisFree(c);
        }
        else
        {
            std::cerr << "Connection error: can't allocate redis context" << std::endl;
        }
        return 1;
    }

    // 执行 SET 命令
    redisReply *reply = (redisReply *)redisCommand(c, "SET mykey hello");
    if (reply == nullptr)
    {
        std::cerr << "SET command failed" << std::endl;
        redisFree(c);
        return 1;
    }
    std::cout << "SET: " << reply->str << std::endl;
    freeReplyObject(reply);

    // 执行 GET 命令
    reply = (redisReply *)redisCommand(c, "GET mykey");
    if (reply->type == REDIS_REPLY_STRING)
    {
        std::cout << "GET mykey: " << reply->str << std::endl;
    }
    else
    {
        std::cout << "GET mykey: (nil)" << std::endl;
    }
    freeReplyObject(reply);

    // 关闭连接
    redisFree(c);
    return 0;
}

int startRedis()
{

    return 0;
}