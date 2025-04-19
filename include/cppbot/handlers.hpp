#ifndef HANDLERS_HPP
#define HANDLERS_HPP

#include <functional>
#include <string>
#include <map>
#include <unordered_map>
#include <utility>
#include "types.hpp"
#include "states.hpp"

namespace handlers
{
  namespace detail
  {
    struct PairHasher
    {
      size_t operator()(const std::pair< std::string, states::State >& p) const
      {
        size_t h1 = std::hash< std::string >{}(p.first);
        size_t h2 = std::hash< states::State >{}(p.second);
        return h1 ^ (h2 << 1);
      }
    };
  }

  class MessageHandler
  {
    using handler_t = std::function< void(const types::Message&) >;
    using state_handler_t = std::function< void(const types::Message&, states::StateContext&) >;
   public:
    MessageHandler() = default;
    void addHandler(const std::string& cmd, handler_t handler);
    void addHandler(const states::State& state, state_handler_t handler);
    void addHandler(const std::string& cmd, const states::State& state, state_handler_t handler);
    void processMessage(const types::Message& msg, states::StateContext& state) const;
   private:
    std::unordered_map< std::string, handler_t > cmdHandlers_;
    std::unordered_map< states::State, state_handler_t > stateHandlers_;
    std::unordered_map< std::pair< std::string, states::State >, state_handler_t, detail::PairHasher > stateCmdHandlers_;
  };

  class CallbackQueryHandler
  {
    using handler_t = std::function< void(const types::CallbackQuery&) >;
   public:
    CallbackQueryHandler() = default;
    void addHandler(const std::string& callData, handler_t handler, bool allowPartialMatch = false);
    void processCallbackQuery(const types::CallbackQuery& query) const;
   private:
    std::unordered_map< std::string, handler_t > handlers_;
    std::map< std::string, handler_t > partialMatchHandlers_;
  };
}

#endif
