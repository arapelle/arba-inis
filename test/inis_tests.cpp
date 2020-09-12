#include <inis/settings.hpp>
#include <gtest/gtest.h>
#include <filesystem>
#include <cstdlib>

std::string program_dir;

TEST(inis_tests, basic_test)
{
    std::filesystem::path inis_filepath("settings.inis");
    ASSERT_TRUE(std::filesystem::exists(inis_filepath));
    inis::settings settings;
    settings.load_from_file(inis_filepath);

    ASSERT_NE(settings.get<std::string>(inis::settings::settings_dir), "");
    ASSERT_NE(settings.get<std::string>(inis::settings::working_dir), "");
    ASSERT_NE(settings.get<std::string>(inis::settings::tmp_dir), "");

    ASSERT_EQ(settings.get<std::string>("app.host"), "localhost");
    ASSERT_EQ(settings.get<int>("app.port"), 4242);
    ASSERT_EQ(settings.get<std::string>("app.description"), "A wonderful application!\nEnjoy!");
    std::string fire_butterfly = settings.get<std::string>("monsters.flying.fire_butterfly");
    ASSERT_EQ(fire_butterfly, "[type.dark_fire] butterfly");
    settings.format(fire_butterfly);
    ASSERT_EQ(fire_butterfly, "dark red butterfly");

    ASSERT_NE(settings.section_ptr("app"), nullptr);
    ASSERT_NE(settings.section_ptr("monsters"), nullptr);
    ASSERT_NE(settings.section_ptr("monsters.flying"), nullptr);
    ASSERT_NE(settings.section_ptr("type"), nullptr);

    settings.print();
}

int main(int argc, char** argv)
{
    program_dir = std::filesystem::canonical(*argv).parent_path().generic_string();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
