#ifndef MSGHANDLER_H
#define MSGHANDLER_H

#include <functional>
#include <string>
#include <tuple>

#include "networkMsg.h"

std::tuple<ClientID, MsgType, std::string> HandleLoginRequest(const ClientID &client, const std::string &message)
{
    // Handle login request
    return std::make_tuple(client, LOGIN_RESPONSE, "Login successful");
}

#endif // MSGHANDLER_H