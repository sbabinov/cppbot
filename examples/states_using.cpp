#include "cppbot/types.hpp"
#include "cppbot/states.hpp"
#include "bot.hpp"
#include "forms.hpp"

// Called when entering command /start
void startRegistration(const types::Message& msg)
{
  auto state = app::bot.getStateContext(msg.chat.id); // get current state context
  app::bot.sendMessage(msg.chat.id, "Hello! Let's start registration. Enter username:");
  state.setState(forms::registrationForm.username); // set new state
}

void processUsername(const types::Message& msg, states::StateContext& state)
{
  state.data(state.current()) = msg.text; // set state context data for current state (registration.username)
  app::bot.sendMessage(msg.chat.id, "Great! Now enter your age:");
  state.setState(forms::registrationForm.age); // set next state
}

void processAge(const types::Message& msg, states::StateContext& state)
{
  state.data(state.current()) = msg.text;
  app::bot.sendMessage(msg.chat.id, "Last question, what's your country?");
  state.setState(forms::registrationForm.country);
}

void finishRegistration(const types::Message& msg, states::StateContext& state)
{
  auto data = state.data(); // get all current data fields
  std::string text = "Successfull registration! Please check all fields are right:\n\n";

  // Now we can read fields that write before
  text += "Username: " + boost::any_cast< std::string >(data[forms::registrationForm.username]) + '\n';
  text += "Age: " + boost::any_cast< std::string >(data[forms::registrationForm.age]) + '\n';
  text += "Country: " + msg.text;

  state.resetState(); // Don't forget to reset the state to default
  app::bot.sendMessage(msg.chat.id, text);
}

int main()
{
  app::messageHandler->addHandler("/start", startRegistration);

  // This handlers will be called only in appropriate states
  app::messageHandler->addHandler(forms::registrationForm.username, processUsername);
  app::messageHandler->addHandler(forms::registrationForm.age, processAge);
  app::messageHandler->addHandler(forms::registrationForm.country, finishRegistration);

  app::bot.startPolling();
  return 0;
}
