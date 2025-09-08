#ifndef MSGHANDLER_H
#define MSGHANDLER_H

#include <functional>
#include <string>
#include <tuple>

#include "networkMsg.h"

std::tuple<ClientID, MsgType, std::string> HandleLoginRequest(const ClientID &client, const std::string &message);

std::tuple<ClientID, MsgType, std::string> HandleSignUpRequest(const ClientID &client, const std::string &message);

#endif // MSGHANDLER_H