#ifndef LPTC_CODERDOJO_COMMAND_H_
#define LPTC_CODERDOJO_COMMAND_H_

#include <iostream>
#include <vector>

namespace lptc_coderdojo {

class Command {
 public:
  enum Action { SUBSCRIBE, UNSUBSCRIBE, INVALID };

  Command(Action a);
  Command(Action a, const std::string t);

  const Action& GetAction() const;
  const std::string& GetTopic() const;

  static std::string ActionStr(Action a);
  static Action ActionFromToken(const std::string& token);
  static std::vector<std::string> GetTokensFromPayload(
      const std::string& msg_payload);
  static Command FromMessagePayload(const std::string& msg);

 private:
  Action action;
  std::string topic;
};

}  // namespace lptc_coderdojo

#endif  // LPTC_CODERDOJO_COMMAND_H_