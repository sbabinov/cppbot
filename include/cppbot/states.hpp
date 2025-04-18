#ifndef STATE_HPP
#define STATE_HPP

#include <string>
#include <memory>
#include <unordered_map>
#include <boost/any.hpp>

namespace states
{
  class State
  {
   public:
    friend struct std::hash< State >;

    State();
    bool operator==(const State& other) const;
   private:
    size_t id_;
    static size_t lastId_;
  };

  class StatesForm
  {
   public:
    using Data = std::unordered_map< State, boost::any >;
  };

  class Storage
  {
   public:
    friend class StateMachine;
    friend class StateContext;

    Storage();
    StatesForm::Data& data(size_t chatId);
    boost::any& data(size_t chatId, const State& state);
    void remove(size_t chatId);
   private:
    std::unordered_map< size_t, State > currentStates_;
    std::unordered_map< size_t, StatesForm::Data > data_;
  };

  class StateMachine
  {
   public:
    friend class StateContext;

    static const State DEFAULT_STATE;
    StateMachine(std::shared_ptr< Storage > storage);
    void setState(size_t chatId, const State& state) const;
    State getState(size_t chatId) const;
   private:
    std::shared_ptr< Storage > storage_;
  };

  class StateContext
  {
   public:
    StateContext(size_t chatId, StateMachine* stateMachine);
    State current();
    void setState(const State& state) const;
    void resetState();
    StatesForm::Data& data();
    boost::any& data(const State& state);
   private:
    size_t chatId_;
    StateMachine* stateMachine_;
  };
}

namespace std
{
  template<>
  struct hash< states::State >
  {
    size_t operator()(const states::State& obj) const
    {
      return obj.id_;
    }
  };
}

#endif
