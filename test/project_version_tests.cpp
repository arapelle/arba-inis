#include <arba/inis/version.hpp>
#include <gtest/gtest.h>
#include <cstdlib>

TEST(project_version_tests, test_version_macros)
{
    ASSERT_EQ(ARBA_INIS_VERSION_MAJOR, 0);
    ASSERT_EQ(ARBA_INIS_VERSION_MINOR, 2);
    ASSERT_EQ(ARBA_INIS_VERSION_PATCH, 0);
    ASSERT_STREQ(ARBA_INIS_VERSION, "0.2.0");
}
