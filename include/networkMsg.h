#ifndef NETWORKMSG_H
#define NETWORKMSG_H

enum MsgType {
  SYSTEM,
  HEARTBEAT,

  LOGIN_REQUEST,
  LOGIN_RESPONSE,
  LOGOUT_RESQUEST,
  LOGOUT_RESPONSE,

  SIGN_UP_RESQUEST,
  SIGN_UP_RESPONSE,

  CHAT_TEXT,
  CHAT_ACK,

  USER_ONLINE,
  USER_OFFLINE,
  USER_TYPEING,
};

struct ClientID
{
    int                                   fd;
    std::chrono::steady_clock::time_point acceptTime;
    uint64_t                              randomValue;

    bool operator==(const ClientID &other) const
    {
        return fd == other.fd && acceptTime == other.acceptTime && randomValue == other.randomValue;
    }
};

#endif  // NETWORKMSG_H
