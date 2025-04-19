#include "cppbot/types.hpp"
#include "bot.hpp"

void openMainMenu(const types::Message& msg)
{
  // Creating an inline keyboard with two buttons
  types::InlineKeyboardMarkup menu({
    {
      types::InlineKeyboardButton("Profile", "open-profile"),
      types::InlineKeyboardButton("Settings", "open-settings")
    }
  });

  // Sending a message with menu keyboard
  app::bot.sendMessage(msg.chat.id, "Main menu", menu);
}

// This handler is called when button with data "open-profile" was pressed
void openProfile(const types::CallbackQuery& query)
{
  app::bot.answerCallbackQuery(query.id); // It's a good practice to answer to a query
  app::bot.sendMessage(query.from.id, "You are in profile");
  // ...
}

// This handler is called when button with data "open-settings" was pressed
void openSettings(const types::CallbackQuery& query)
{
  app::bot.answerCallbackQuery(query.id);
  app::bot.sendMessage(query.from.id, "You are in settings");
  // ...
}

int main()
{
  app::messageHandler->addHandler("/menu", openMainMenu);

  // This handlers will be called when buttons with appropriate data are pressed
  app::queryHandler->addHandler("open-profile", openProfile);
  app::queryHandler->addHandler("open-settings", openSettings);

  app::bot.startPolling();

  return 0;
}
