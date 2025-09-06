#include "msgHandler.h"

#include "databaseManager.h"
#include "msg.pb.h"
#include "msg_header.pb.h"
#include "networkMsg.h"

#include <iostream>

msg_header::ServerMsgHeader GetServerMsgHeader()
{
    msg_header::ServerMsgHeader header;
    header.set_timestamp(
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    return header;
}

std::tuple<ClientID, MsgType, std::string> GetInvalidMessageErrorResponse(const ClientID &client)
{
    msg::InvalidMessageError invalidMsgError;
    invalidMsgError.mutable_header()->CopyFrom(GetServerMsgHeader());
    std::string msg;
    invalidMsgError.SerializeToString(&msg);
    return std::make_tuple(client, INVALID_MESSAGE_ERROR, msg);
}

std::tuple<ClientID, MsgType, std::string> HandleLoginRequest(const ClientID &client, const std::string &message)
{
    // Parse the message using protobuf
    msg::LoginRequest loginReq;
    if (!loginReq.ParseFromString(message))
    {
        return GetInvalidMessageErrorResponse(client);
    }

    // Check credentials
    DatabaseManager::ResultCode result = DatabaseManager::instance()->authenticateUser(loginReq.username(), loginReq.password());

    // Prepare response
    msg::LoginResponse loginResp;
    loginResp.mutable_header()->CopyFrom(GetServerMsgHeader());
    if (result == DatabaseManager::USER_VERIFICATION_SUCCESS)
    {
        loginResp.set_state(msg::LoginResponse::USER_VERIFICATION_SUCCESS);
        loginResp.set_session_id("session_abc123");
    }
    else
    {
        loginResp.set_state(msg::LoginResponse::USER_VERIFICATION_FAILED);
        loginResp.set_session_id("");
    }
    std::string msg;
    loginResp.SerializeToString(&msg);

    return std::make_tuple(client, LOGIN_RESPONSE, msg);
}
