#ifndef NETWORKMSG_H
#define NETWORKMSG_H

#include <chrono>

enum MsgType {
  SYSTEM,
  HEARTBEAT,
  INVALID_MESSAGE_ERROR,

  LOGIN_REQUEST,
  LOGIN_RESPONSE,
  LOGOUT_REQUEST,
  LOGOUT_RESPONSE,

  SIGN_UP_REQUEST,
  SIGN_UP_RESPONSE,

  CHAT_TEXT,
  CHAT_ACK,

  USER_ONLINE,
  USER_OFFLINE,
  USER_TYPEING,
};

struct ClientID
{
    std::chrono::steady_clock::time_point acceptTime;
    uint64_t                              randomValue;

    bool operator==(const ClientID &other) const
    {
        return acceptTime == other.acceptTime && randomValue == other.randomValue;
    }
};

#endif  // NETWORKMSG_H
