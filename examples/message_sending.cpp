#include "cppbot/types.hpp"
#include "bot.hpp"

// Sync sending
void echoSync(const types::Message& msg)
{
  // Sending a message
  cppbot::Bot::futureMessage future = app::bot.sendMessage(msg.chat.id, "Welcome, " + msg.from.firstName + '!');
  types::Message res = future.get(); // Execution will block waiting for completion of sending
  // Now you can use returned message
}

// Async sending
void echoAsync(const types::Message& msg)
{
  // Sending a message
  app::bot.sendMessage(msg.chat.id, "Welcome, " + msg.from.firstName + '!');
  // ... (next code will execute without waiting for completion of sending)
}

int main()
{
  app::messageHandler->addHandler("/echoSync", echoSync);
  app::messageHandler->addHandler("/echoAsync", echoAsync);
  app::bot.startPolling();
  return 0;
}
