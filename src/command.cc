#include "command.h"

#include <sstream>

namespace lptc_coderdojo {

Command::Command(Action a) : action(a) {}
Command::Command(Action a, const std::string t) : action(a), topic(t) {}

const Command::Action& Command::GetAction() const { return action; }
const std::string& Command::GetTopic() const { return topic; }

std::string Command::ActionStr(Action a) {
  switch (a) {
    case Action::SUBSCRIBE:
      return "SUBSCRIBE";
    case Action::UNSUBSCRIBE:
      return "UNSUBSCRIBE";
    case Action::INVALID:
    default:
      return "INVALID";
  }
}

Command::Action Command::ActionFromToken(const std::string& token) {
  if (token.compare(ActionStr(Action::SUBSCRIBE)) == 0) {
    return Action::SUBSCRIBE;
  } else if (token.compare(ActionStr(Action::UNSUBSCRIBE)) == 0) {
    return Action::UNSUBSCRIBE;
  } else {
    return Action::INVALID;
  }
}

std::vector<std::string> Command::GetTokensFromPayload(
    const std::string& msg_payload) {
  std::istringstream iss(msg_payload);
  std::vector<std::string> tokens;
  std::string token;

  while (std::getline(iss, token, ' '))
    if (!token.empty()) tokens.push_back(token);

  return tokens;
}

Command Command::FromMessagePayload(const std::string& msg) {
  std::vector<std::string> tokens = GetTokensFromPayload(msg);

  if (tokens.size() != 2 || tokens[0].length() == 0 || tokens[1].length() == 0)
    return Command(Action::INVALID);

  return Command(ActionFromToken(tokens[0]), tokens[1]);
}

}  // namespace lptc_coderdojo