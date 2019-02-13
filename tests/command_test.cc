#include <gtest/gtest.h>

#include "command.h"

namespace {

TEST(ServerCommandTest, FromMessagePayload_InvalidAction) {
  {
    lptc_coderdojo::Command c =
        lptc_coderdojo::Command::FromMessagePayload("invalid_command");
    EXPECT_EQ(lptc_coderdojo::Command::Action::INVALID, c.GetAction());
  }
  {
    lptc_coderdojo::Command c =
        lptc_coderdojo::Command::FromMessagePayload("invalid_command topic");
    EXPECT_EQ(lptc_coderdojo::Command::Action::INVALID, c.GetAction());
  }
  {
    lptc_coderdojo::Command c = lptc_coderdojo::Command::FromMessagePayload("");
    EXPECT_EQ(lptc_coderdojo::Command::Action::INVALID, c.GetAction());
  }
  {
    lptc_coderdojo::Command c =
        lptc_coderdojo::Command::FromMessagePayload("subscribe test_topic");
    EXPECT_EQ(lptc_coderdojo::Command::Action::INVALID, c.GetAction());
  }
  {
    lptc_coderdojo::Command c =
        lptc_coderdojo::Command::FromMessagePayload("SUBSCRIBEtest_topic");
    EXPECT_EQ(lptc_coderdojo::Command::Action::INVALID, c.GetAction());
  }
}

TEST(ServerCommandTest, FromMessagePayload_ValidAction) {
  {
    lptc_coderdojo::Command c =
        lptc_coderdojo::Command::FromMessagePayload("SUBSCRIBE test_topic");
    EXPECT_EQ(lptc_coderdojo::Command::Action::SUBSCRIBE, c.GetAction());
    EXPECT_EQ("test_topic", c.GetTopic());
  }
  {
    lptc_coderdojo::Command c =
        lptc_coderdojo::Command::FromMessagePayload("UNSUBSCRIBE test_topic");
    EXPECT_EQ(lptc_coderdojo::Command::Action::UNSUBSCRIBE, c.GetAction());
    EXPECT_EQ("test_topic", c.GetTopic());
  }
}

}  // namespace