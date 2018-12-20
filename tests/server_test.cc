#include <gtest/gtest.h>

#include "server.h"

namespace {
TEST(ServerCommandTest, ActionFromToken_InvalidAction) {
  lptc_coderdojo::Command c =
      lptc_coderdojo::Command::ActionFromToken("invalid command");
  EXPECT_EQ(lptc_coderdojo::Command::Action::INVALID, c.GetAction());
}

TEST(ServerCommandTest, ActionFromToken_ValidSubscribeAction) {
  lptc_coderdojo::Command c =
      lptc_coderdojo::Command::ActionFromToken("SUBSCRIBE test_topic");
  EXPECT_EQ(lptc_coderdojo::Command::Action::SUBSCRIBE, c.GetAction());
}
}  // namespace