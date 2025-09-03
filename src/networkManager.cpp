#include "networkManager.h"

uint64_t GenerateToken()
{
    static std::mt19937_64                         rng(std::random_device{}());
    static std::uniform_int_distribution<uint64_t> dist;
    return dist(rng);
}

NetworkManager::NetworkManager() {}

NetworkManager::~NetworkManager()
{
    // Stop the thread pool
    m_threadPoolStop = true;
    m_condition.notify_all();
    for (auto &worker : m_workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }

    // Close all client connections
    for (auto &pair : m_ClientIDToEpollData)
    {
        closeConnection(pair.second);
    }

    // Clear message queues
    {
        std::lock_guard<std::mutex> lock(m_readMessageQueueMutex);
        while (!m_readMessageQueue.empty())
        {
            m_readMessageQueue.pop();
        }
    }
    {
        std::lock_guard<std::mutex> lock(m_sendMessageQueueMutex);
        while (!m_sendMessageQueue.empty())
        {
            m_sendMessageQueue.pop();
        }
    }
}

NetworkManager *NetworkManager::instance()
{
    static NetworkManager *instance = nullptr;
    if (!instance)
    {
        instance = new NetworkManager();
    }
    return instance;
}

void NetworkManager::setPort(uint32_t port)
{
    m_port = port;
}

void NetworkManager::addMessageHandler(
    MsgType                                                                                         msgType,
    std::function<std::tuple<ClientID, MsgType, std::string>(ClientID client, const std::string &)> handler)
{
    m_msgHandlers[msgType] = handler;
}

void NetworkManager::removeMessageHandler(MsgType msgType)
{
    m_msgHandlers.erase(msgType);
}

void NetworkManager::start(uint32_t port)
{
    // If port is not set, use the provided port
    if (m_port == 0)
    {
        m_port = port;
    }

    // Create server socket
    m_serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverFd == -1)
    {
        throw std::runtime_error("Failed to create socket");
    }

    // Set socket options: SO_REUSEPORT
    int opt = 1;
    if (setsockopt(m_serverFd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    {
        close(m_serverFd);
        throw std::runtime_error("Set socket option SO_REUSEPORT failed");
    }

    // Bind server socket to port
    sockaddr_in serverAddr{};
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port        = htons(m_port);
    if (bind(m_serverFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(m_serverFd, SOMAXCONN) == -1)
    {
        throw std::runtime_error("Failed to listen on socket");
    }

    // Create epoll instance
    m_epollFd = epoll_create1(0);
    if (m_epollFd == -1)
    {
        throw std::runtime_error("Failed to create epoll instance");
    }

    // Add server socket to epoll
    struct epoll_event event;
    event.events  = EPOLLIN;
    event.data.fd = m_serverFd;
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_serverFd, &event) == -1)
    {
        throw std::runtime_error("Failed to add server socket to epoll");
    }

    // Initialize thread pool
    initThreadPool();

    struct epoll_event events[m_maxEpollEvents];
    while (true)
    {
        int nready = epoll_wait(m_epollFd, events, m_maxEpollEvents, -1);
        if (nready < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            throw std::runtime_error("Failed to wait on epoll");
        }

        for (int i = 0; i < nready; ++i)
        {
            if (events[i].data.fd == m_serverFd)
            {
                // Accept new connection
                sockaddr_in clientAddr;
                socklen_t   clientAddrLen = sizeof(clientAddr);
                int         clientFd      = accept(m_serverFd, (struct sockaddr *)&clientAddr, &clientAddrLen);
                if (clientFd == -1)
                {
                    continue; // Accept failed, skip this iteration
                }

                // Create epoll event for new client socket
                struct epoll_event clientEvent;

                // Set up client event
                clientEvent.events = EPOLLIN | EPOLLIN; // Edge-triggered

                // Set up EpollData
                EpollData *data      = new EpollData;
                data->fd             = clientFd;
                data->port           = ntohs(clientAddr.sin_port);
                data->ip             = inet_ntoa(clientAddr.sin_addr);
                data->lastActiveTime = std::chrono::steady_clock::now();
                data->callback       = [this](epoll_event &event) { this->epollCallback(event); };
                clientEvent.data.ptr = data;

                // Set socket to non-blocking
                int flags = fcntl(clientFd, F_GETFL, 0);
                fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

                // Add new client socket to epoll
                if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, clientFd, &clientEvent) == -1)
                {
                    close(clientFd);
                    continue; // Failed to add client socket to epoll, skip this iteration
                }

                // Map ClientID to EpollData
                ClientID ClientIDKey;
                ClientIDKey.fd                     = clientFd;
                ClientIDKey.acceptTime             = data->lastActiveTime;
                ClientIDKey.randomValue            = GenerateToken();
                m_ClientIDToEpollData[ClientIDKey] = data;

                // Map EpollData to ClientID
                m_EpollDataToClientID[data] = ClientIDKey;
            }
            else
            {
                // Handle client socket event
                events[i].data.ptr ? epollCallback(events[i]) : void();
            }
        }
        // Process read message queue
        {
            std::lock_guard<std::mutex> lock(m_readMessageQueueMutex);
            for (auto it : m_ClientIDToEpollData)
            {
                EpollData  *data = it.second;
                MsgType     msgType;
                std::string msg;
                while (readMessage(data, msgType, msg))
                {
#ifdef DEBUG
                    std::cout << "Received message from client (fd=" << it.first.fd << "): type=" << msgType
                              << ", length=" << msg.size() << std::endl;
#endif
                    m_readMessageQueue.emplace(it.first, msgType, msg);
                    m_condition.notify_one();
                }
            }
        }
        // Process send message queue
        {
            std::lock_guard<std::mutex> lock(m_sendMessageQueueMutex);
            while (!m_sendMessageQueue.empty())
            {
                auto pair = m_sendMessageQueue.front();

                ClientID           clientID = std::get<0>(pair);
                MsgType            msgType  = std::get<1>(pair);
                const std::string &packet   = std::get<2>(pair);

                sendMessage(clientID, msgType, packet);

                m_sendMessageQueue.pop();
            }
        }
    }
}

void NetworkManager::closeConnection(EpollData *data)
{
    if (data)
    {
#ifdef DEBUG
        std::cout << "Closing connection (fd=" << data->fd << ")" << std::endl;
#endif
        m_ClientIDToEpollData.erase({m_EpollDataToClientID[data]});
        m_EpollDataToClientID.erase(data);
        epoll_ctl(m_epollFd, EPOLL_CTL_DEL, data->fd, nullptr);
        close(data->fd);
        delete data;
        data = nullptr;
    }
}

void NetworkManager::epollCallback(epoll_event &event)
{
    EpollData *data     = static_cast<EpollData *>(event.data.ptr);
    const int  clientFd = data->fd;

    // Handle read event
    if (event.events & EPOLLIN)
    {
        char buffer[4096];
        while (true)
        {
            ssize_t n = read(clientFd, buffer, sizeof(buffer));
            if (n > 0)
            {
                data->readBuffer.append(std::string(buffer, n));
            }
            else if (n == 0)
            {
                closeConnection(data);
                return;
            }
            else
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break;
                }
                else
                {
                    closeConnection(data);
                    return;
                }
            }
        }
    }

    // Handle write event
    if (event.events & EPOLLOUT)
    {
        while (!data->writeBuffer.empty())
        {
            ssize_t n = write(data->fd, data->writeBuffer.data(), data->writeBuffer.size());
            if (n > 0)
            {
                data->writeBuffer.erase(0, n);
            }
            else
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break;
                }
                else
                {
                    closeConnection(data);
                    return;
                }
            }
        }
    }

    // Trigger write event
    event.events = event.events | EPOLLIN;
    epoll_ctl(m_epollFd, EPOLL_CTL_MOD, clientFd, &event);
}

bool NetworkManager::readMessage(EpollData *data, MsgType &msgType, std::string &msg)
{
    // Loop to parse complete messages in the buffer
    while (true)
    {
        // Check if the buffer contains at least the message header (type + length)
        if (data->readBuffer.size() < sizeof(uint16_t) + sizeof(uint32_t))
        {
            // Incomplete data, wait for the next receive
            break;
        }

        // Read message type
        uint16_t typeVal;
        data->readBuffer.copy(reinterpret_cast<char *>(&typeVal), sizeof(typeVal), 0);
        typeVal = ntohs(typeVal); // Convert from network byte order to host byte order
        msgType = static_cast<MsgType>(typeVal);

        // Read message length
        uint32_t msgLen;
        data->readBuffer.copy(reinterpret_cast<char *>(&msgLen), sizeof(msgLen), sizeof(typeVal));
        msgLen = ntohl(msgLen); // Convert from network byte order to host byte order

        // Check if the buffer contains the complete message body
        if (data->readBuffer.size() < sizeof(typeVal) + sizeof(msgLen) + msgLen)
        {
            // Message not fully received yet, wait for more data
            break;
        }

        // Extract message content
        msg.assign(data->readBuffer.data() + sizeof(typeVal) + sizeof(msgLen), msgLen);

        // Remove the processed message from the buffer
        data->readBuffer.erase(0, sizeof(typeVal) + sizeof(msgLen) + msgLen);

        // Successfully extracted one complete message, exit function
        return true;
    }
    return false;
}

void NetworkManager::sendMessage(const ClientID &clientID, MsgType msgType, const std::string &msg)
{
    // Prepare packet with header (type + length)
    std::string packet;
    uint16_t    typeVal = htons(static_cast<uint16_t>(msgType));
    uint32_t    msgLen  = htonl(static_cast<uint32_t>(msg.size()));
    packet.append(reinterpret_cast<const char *>(&typeVal), sizeof(typeVal));
    packet.append(reinterpret_cast<const char *>(&msgLen), sizeof(msgLen));
    packet.append(msg);

    auto it = m_ClientIDToEpollData.find(clientID);
    // Client found
    if (it != m_ClientIDToEpollData.end())
    {
        it->second->writeBuffer.append(packet);
        // Trigger write event
        epoll_event event;
        event.events   = event.events | EPOLLOUT;
        event.data.ptr = it->second;
        epoll_ctl(m_epollFd, EPOLL_CTL_MOD, clientID.fd, &event);
#ifndef DEBUG
        std::cout << "Sending message to client (fd=" << clientID.fd << "): type=" << msgType
                  << ", length=" << msg.size() << std::endl;
#endif
    }
    // Client not found, possibly disconnected
    /*
    else
    {

    }
    */
}

void NetworkManager::setMaxWorkerThreads(size_t maxThreads)
{
    m_maxWorkerThreads = maxThreads;
}

void NetworkManager::initThreadPool()
{
    // Determine the maximum number of worker threads
    if (m_maxWorkerThreads == 0)
    {
        m_maxWorkerThreads = std::thread::hardware_concurrency();
    }

#ifdef DEBUG
    std::cout << "Initializing thread pool with " << m_maxWorkerThreads << " threads." << std::endl;
#endif

    // Create worker threads
    for (size_t i = 0; i < m_maxWorkerThreads; ++i)
    {
        m_workers.emplace_back([this]() {
            while (true)
            {
                // Wait for tasks
                std::tuple<ClientID, MsgType, std::string> task;
                {
                    std::unique_lock<std::mutex> lock(m_readMessageQueueMutex);
                    m_condition.wait(lock, [this] { return m_threadPoolStop || !m_readMessageQueue.empty(); });
                    if (m_threadPoolStop && m_readMessageQueue.empty()) return;
                    task = std::move(m_readMessageQueue.front());
                    m_readMessageQueue.pop();
                }

                // Find message handler
                auto it = m_msgHandlers.find(std::get<1>(task));
                if (it != m_msgHandlers.end())
                {
                    // Call the message handler
                    std::tuple<ClientID, MsgType, std::string> response =
                        it->second(std::get<0>(task), std::get<2>(task));

                    // Check if response is unempty
                    if (std::get<2>(response).size())
                    {
                        // Push response to send message queue
                        std::unique_lock<std::mutex> lock(m_sendMessageQueueMutex);
                        m_sendMessageQueue.push(response);
                    }
                }
            }
        });
    }
}