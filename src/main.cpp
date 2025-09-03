#include <iostream>

#include "networkManager.h"
#include "msgHandler.h"

int main() {
  std::cout << "Hello, this is SecureTalk server!" << std::endl;

  // Register message handlers
  NetworkManager::instance()->addMessageHandler(LOGIN_REQUEST, HandleLoginRequest);

  // Start the network manager
  NetworkManager::instance()->start(7777);

  return 0;
}