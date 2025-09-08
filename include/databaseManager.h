#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <filesystem>
#include <iomanip>
#include <mutex>
#include <openssl/sha.h>
#include <random>
#include <sqlite3.h>
#include <sstream>
#include <stdexcept>
#include <string>

class DatabaseManager
{
  public:
    enum ResultCode
    {
        SUCCESS = 0,
        DATABASE_ERROR,
        USER_ALREADY_EXISTS,
        USER_NOT_FOUND,
        INVALID_PASSWORD
    };

  public:
    static DatabaseManager *instance();

  public:
    DatabaseManager();
    ~DatabaseManager();

    ResultCode createUser(const std::string &username, const std::string &password);
    ResultCode authenticateUser(const std::string &username, const std::string &password);
    ResultCode deleteUser(const std::string &username);

  private:
    std::mutex            m_user_databaseMutex;
    std::mutex            m_msg_databaseMutex;
    sqlite3              *m_user_database     = nullptr;
    sqlite3              *m_msg_database      = nullptr;
    std::filesystem::path m_user_databasePath = "../data/user.db";
    std::filesystem::path m_msg_databasePath = "../data/msg.db";
};

#endif // DATABASE_MANAGER_H