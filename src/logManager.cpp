#include "logManager.h"

std::shared_ptr<Logger> logger         = std::make_shared<Logger>("logger");
std::shared_ptr<Logger> networkLogger  = std::make_shared<Logger>("networkLogger");
std::shared_ptr<Logger> databaseLogger = std::make_shared<Logger>("databaseLogger");
std::shared_ptr<Logger> handlerLogger  = std::make_shared<Logger>("handlerLogger");

void InitLogger()
{
    auto consoleAppender         = std::make_shared<ConsoleLogAppender>(LogLevel::DEBUG);
    auto completeDebugAppender   = std::make_shared<FileLogAppender>("../log/completeDebugLog", LogLevel::DEBUG);
    auto completeRuntimeAppender = std::make_shared<FileLogAppender>("../log/completeRuntimeLog", LogLevel::INFO);
    auto networkAppender         = std::make_shared<FileLogAppender>("../log/networkLog", LogLevel::INFO);
    auto databaseAppender        = std::make_shared<FileLogAppender>("../log/databaseLog", LogLevel::INFO);
    auto handlerAppender         = std::make_shared<FileLogAppender>("../log/handlerLog", LogLevel::INFO);

    logger->addAppender(consoleAppender);
    logger->addAppender(completeDebugAppender);
    logger->addAppender(completeRuntimeAppender);

    networkLogger->addAppender(networkAppender);
    networkLogger->addAppender(consoleAppender);
    networkLogger->addAppender(completeDebugAppender);
    networkLogger->addAppender(completeRuntimeAppender);

    databaseLogger->addAppender(databaseAppender);
    databaseLogger->addAppender(consoleAppender);
    databaseLogger->addAppender(completeDebugAppender);
    databaseLogger->addAppender(completeRuntimeAppender);

    handlerLogger->addAppender(handlerAppender);
    handlerLogger->addAppender(consoleAppender);
    handlerLogger->addAppender(completeDebugAppender);
    handlerLogger->addAppender(completeRuntimeAppender);
}
