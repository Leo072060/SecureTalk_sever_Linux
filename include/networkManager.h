#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <arpa/inet.h>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <tuple>
#include <unistd.h>
#include <unordered_map>

#include "networkMsg.h"

// Provide hash function for unordered_map
namespace std
{
template <> struct hash<ClientID>
{
    size_t operator()(const ClientID &c) const
    {
        size_t h1 = std::hash<int>()(c.fd);
        size_t h2 = std::hash<uint64_t>()(c.randomValue);
        // Hash steady_clock::time_point as duration count
        size_t h3 = std::hash<int64_t>()(c.acceptTime.time_since_epoch().count());

        // combine hashes
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
} // namespace std

class NetworkManager
{
  private:
    struct EpollData
    {
        int                                   fd;
        uint16_t                              port;
        std::string                           ip;
        std::string                           readBuffer;
        std::string                           writeBuffer;
        std::chrono::steady_clock::time_point lastActiveTime;
        std::function<void(epoll_event &)>    callback;
    };

  private:
    explicit NetworkManager();

  public:
    ~NetworkManager();

  public:
    static NetworkManager *instance();

    void setPort(uint32_t port);
    void addMessageHandler(
        MsgType                                                                                         msgType,
        std::function<std::tuple<ClientID, MsgType, std::string>(ClientID client, const std::string &)> handler);
    void removeMessageHandler(MsgType msgType);
    void start(uint32_t port);

  private:
    void closeConnection(EpollData *data);
    void epollCallback(epoll_event &event);
    bool readMessage(EpollData *data, MsgType &msgType, std::string &msg);
    void sendMessage(const ClientID &clientID, MsgType msgType, const std::string &msg);

  public:
    void setMaxWorkerThreads(size_t maxThreads);

  private:
    void initThreadPool();

  private:
    uint32_t m_port           = 0;
    int      m_serverFd       = 0;
    int      m_epollFd        = 0;
    int      m_maxEpollEvents = 1024;

    std::unordered_map<ClientID, EpollData *> m_ClientIDToEpollData;
    std::unordered_map<EpollData *, ClientID> m_EpollDataToClientID;
    std::unordered_map<
        MsgType, std::function<std::tuple<ClientID, MsgType, std::string>(const ClientID &client, const std::string &)>>
        m_msgHandlers;

  private:
    std::mutex                                             m_readMessageQueueMutex;
    std::mutex                                             m_sendMessageQueueMutex;
    std::queue<std::tuple<ClientID, MsgType, std::string>> m_readMessageQueue;
    std::queue<std::tuple<ClientID, MsgType, std::string>> m_sendMessageQueue;

  private:
    std::vector<std::thread> m_workers;
    std::condition_variable  m_condition;
    size_t                   m_maxWorkerThreads = 0;
    bool                     m_threadPoolStop   = false;
};

#endif // NETWORKMANAGER_H