#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <memory>

#include "logger.h"

#define PtrToLogger

extern std::shared_ptr<Logger> logger;
extern std::shared_ptr<Logger> networkLogger;
extern std::shared_ptr<Logger> databaseLogger;
extern std::shared_ptr<Logger> handlerLogger;

void InitLogger();

#endif // LOG_MANAGER_H