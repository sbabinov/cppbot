#ifndef STATES_HPP
#define STATES_HPP

#include "cppbot/states.hpp"

namespace forms
{
  struct RegistrationForm: public states::StatesForm
  {
    states::State username;
    states::State age;
    states::State country;
  } registrationForm;
}

#endif
