#include <iostream>
#include <string>

#include "msgHandler.h"
#include "networkManager.h"
#include "logger.h"

void InitDatabase();
int  main()
{
    std::cout << "Hello, this is SecureTalk server!" << std::endl;

    // Create a logger
    Logger logger("SecureTalk");

    // Add console appender
    auto consoleAppender = std::make_shared<ConsoleLogAppender>(LogLevel::DEBUG, LogFormatter("%d [%p] [%t] %f:%l %m%n"));
    logger.addAppender(consoleAppender);

    LOG_DEBUG(logger, "Logger initialized.");
    LOG_INFO(logger, "Starting SecureTalk server...");
    LOG_WARN(logger, "This is a warning message.");
    LOG_ERROR(logger, "This is an error message.");
    LOG_FATAL(logger, "This is a fatal message.");

    // Initialize database
    InitDatabase();

    // Register message handlers
    NetworkManager::instance()->addMessageHandler(LOGIN_REQUEST, HandleLoginRequest);

    // Start the network manager
    NetworkManager::instance()->start(7777);

    return 0;
}

void InitDatabase()
{
    
}