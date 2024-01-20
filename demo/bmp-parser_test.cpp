#include <gtest/gtest.h>
#include "bmp-parser.hpp"

TEST(Parser,GetBytes)
{
    std::string s1 = "0x01, 0x02, 0xff, 0x78, 0x90, 0xab, 0xcd, 0xef";

    static char bytes[1024];
    file_to_bytes(s1,bytes);
    EXPECT_EQ(0x01, bytes[0]);
    EXPECT_EQ(0x02, bytes[1]);
    EXPECT_EQ(0xff, bytes[2]);
}

// Demonstrate some basic assertions.
TEST(HelloTest, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}
