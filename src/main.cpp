#include <iostream>
#include <string>

#include "msgHandler.h"
#include "networkManager.h"
#include "logManager.h"

void InitDatabase();
int  main()
{
    // Initialize loggers
    InitLogger();

    LOG_INFO(logger, "Hello, this is SecureTalk server!");

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