#ifndef BOT_CPP
#define BOT_CPP

#include <memory>
#include "cppbot/cppbot.hpp"
#include "cppbot/states.hpp"
#include "cppbot/handlers.hpp"

namespace app
{
  std::shared_ptr< handlers::MessageHandler > messageHandler = std::make_shared< handlers::MessageHandler >();
  std::shared_ptr< handlers::CallbackQueryHandler > queryHandler = std::make_shared< handlers::CallbackQueryHandler>();
  std::shared_ptr< states::Storage > storage = std::make_shared< states::Storage >();
  cppbot::Bot bot("YOUR_TOKEN_HERE", messageHandler, queryHandler, storage);
}

#endif
