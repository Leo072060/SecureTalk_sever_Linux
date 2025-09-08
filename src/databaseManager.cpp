#include "databaseManager.h"
#include "logManager.h"
std::string generateSalt(size_t length)
{
    static const char charset[] = "0123456789"
                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                  "abcdefghijklmnopqrstuvwxyz"
                                  "!@#$%^&*()_+";

    std::random_device              rd;
    std::mt19937                    gen(rd());
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);

    std::string salt;
    for (size_t i = 0; i < length; ++i)
    {
        salt.push_back(charset[dist(gen)]);
    }
    return salt;
}

std::string sha256(const std::string &input)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(input.c_str()), input.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        ss << std::dec;
    }
    return ss.str();
}

DatabaseManager *DatabaseManager::instance()
{
    static DatabaseManager *instance = nullptr;
    if (!instance)
    {
        instance = new DatabaseManager();
    }
    return instance;
}

DatabaseManager::DatabaseManager()
{
    // Create database if not exists
    if (sqlite3_open(m_user_databasePath.string().c_str(), &m_user_database) != SQLITE_OK)
    {
        LOG_ERROR(databaseLogger, "Failed to open user database");
        throw std::runtime_error("Failed to open user database");
    }
    const char *sql_createUserTable = "CREATE TABLE IF NOT EXISTS users ("
                                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                      "username TEXT UNIQUE NOT NULL,"
                                      "password_hash TEXT NOT NULL,"
                                      "salt TEXT NOT NULL"
                                      ");";
    char       *errMsg              = nullptr;
    if (sqlite3_exec(m_user_database, sql_createUserTable, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        std::string error = "Failed to create users table: ";
        error += errMsg;
        sqlite3_free(errMsg);
        sqlite3_close(m_user_database);
        LOG_ERROR(databaseLogger, error);
        throw std::runtime_error(error);
    }
}

DatabaseManager::~DatabaseManager()
{
    if (m_user_database)
    {
        sqlite3_close(m_user_database);
    }
    if(m_msg_database)
    {
        sqlite3_close(m_msg_database);
    }
}

DatabaseManager::ResultCode DatabaseManager::createUser(const std::string &username, const std::string &password)
{
    // Generate salt and hash password
    std::string salt          = generateSalt(16);
    std::string password_hash = sha256(password + salt);

    // Insert user into database
    std::lock_guard<std::mutex> lock(m_user_databaseMutex);

    // Check if user already exists
    const char   *sql_checkUser = "SELECT COUNT(*) FROM users WHERE username = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(m_user_database, sql_checkUser, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR(databaseLogger, "Failed to prepare SQL statement: " + std::string(sql_checkUser));
        return DATABASE_ERROR;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return DATABASE_ERROR;
    }
    int count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    if (count > 0)
    {
        return USER_ALREADY_EXISTS;
    }

    // Insert new user
    const char *sql_insertUser = "INSERT INTO users (username, password_hash, salt) VALUES (?, ?, ?);";
    if (sqlite3_prepare_v2(m_user_database, sql_insertUser, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR(databaseLogger, "Failed to prepare SQL statement: " + std::string(sql_insertUser));
        return DATABASE_ERROR;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, salt.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        return DATABASE_ERROR;
    }
    sqlite3_finalize(stmt);
    return SUCCESS;
}

DatabaseManager::ResultCode DatabaseManager::authenticateUser(const std::string &username, const std::string &password)
{
    // Retrieve stored hash and salt
    std::string stored_hash;
    std::string salt;
    {
        std::lock_guard<std::mutex> lock(m_user_databaseMutex);

        // Retrieve user info
        const char   *sql_getUser = "SELECT password_hash, salt FROM users WHERE username = ?;";
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(m_user_database, sql_getUser, -1, &stmt, nullptr) != SQLITE_OK)
        {
            LOG_ERROR(databaseLogger, "Failed to prepare statement: " + std::string(sql_getUser));
            return DATABASE_ERROR;
        }
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_ROW)
        {
            sqlite3_finalize(stmt);
            return USER_NOT_FOUND;
        }
        stored_hash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        salt        = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        sqlite3_finalize(stmt);
    }

    // Verify password
    std::string computed_hash = sha256(password + salt);
    if (computed_hash != stored_hash)
    {
        return INVALID_PASSWORD;
    }
    return SUCCESS;
}

DatabaseManager::ResultCode DatabaseManager::deleteUser(const std::string &username)
{
    std::lock_guard<std::mutex> lock(m_user_databaseMutex);

    // Delete user
    const char   *sql_deleteUser = "DELETE FROM users WHERE username = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(m_user_database, sql_deleteUser, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return DATABASE_ERROR;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        return DATABASE_ERROR;
    }
    sqlite3_finalize(stmt);
    return SUCCESS;
}
