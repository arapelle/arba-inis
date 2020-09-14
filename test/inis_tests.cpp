#include <inis/settings.hpp>
#include <gtest/gtest.h>
#include <filesystem>
#include <cstdlib>

std::string program_dir;

TEST(inis_tests, basic_settings_test)
{
    std::filesystem::path inis_filepath("inis/basic_settings.inis");
    ASSERT_TRUE(std::filesystem::exists(inis_filepath));
    inis::settings settings;
    settings.load_from_file(inis_filepath);

    // standard settings:
    ASSERT_EQ(inis::settings::settings_dir, "$settings_dir");
    ASSERT_EQ(inis::settings::working_dir, "$working_dir");
    ASSERT_EQ(inis::settings::tmp_dir, "$tmp_dir");
    ASSERT_NE(settings.get<std::string>(inis::settings::settings_dir), "");
    ASSERT_NE(settings.get<std::string>(inis::settings::working_dir), "");
    ASSERT_NE(settings.get<std::string>(inis::settings::tmp_dir), "");

    // sections:
    ASSERT_EQ(settings.section_ptr(""), &settings);
    ASSERT_NE(settings.section_ptr("section"), nullptr);
    ASSERT_NE(settings.section_ptr("section.subsection"), nullptr);
    ASSERT_NE(settings.section_ptr("section.subsection2"), nullptr);
    ASSERT_EQ(settings.section_ptr("section.subsection.arg"), nullptr);

    // user settings:
    ASSERT_EQ(settings.get<std::string>("global_label"), "value");
    ASSERT_EQ(settings.get<std::string>("section.level"), "0");
    ASSERT_EQ(settings.get<int>("section.level"), 0);
    ASSERT_EQ(settings.get<std::string>("section.arg"), "Text on\nseveral lines.");
    ASSERT_EQ(settings.get<std::string>("section.subsection.arg"), "45.5");
    ASSERT_DOUBLE_EQ(settings.get<double>("section.subsection.arg"), 45.5);
    ASSERT_EQ(settings.get<std::string>("section.subsection2.arg"), "46.5");
    ASSERT_DOUBLE_EQ(settings.get<double>("section.subsection2.arg"), 46.5);
    ASSERT_EQ(settings.get<std::string>("bad_int"), "45a");
    ASSERT_EQ(settings.get<int>("bad_int", -1), -1);
//    ASSERT_EQ(settings.get<std::string>("section.subsection2.last_label", "err"), "err");
//    ASSERT_EQ(settings.get<std::string>("last_label"), "end");
}

TEST(inis_tests, format_test)
{
    std::filesystem::path inis_filepath("inis/settings.inis");
    ASSERT_TRUE(std::filesystem::exists(inis_filepath));
    inis::settings settings;
    settings.load_from_file(inis_filepath);

    // comment:
    std::string value = settings.get<std::string>("comment");
    ASSERT_EQ(value, "[Version: '[version]']");
    settings.format(value);
    ASSERT_EQ(value, "[Version: '0.1.0']");

    // get_formatted("vfs.img"):
    ASSERT_EQ(settings.get_formatted("vfs.img"), "resource/image");

    // get<std::string>("vfs.img"), format(vfs_img):
    std::string vfs_img = settings.get<std::string>("vfs.img");
    ASSERT_EQ(vfs_img, "[vfs.rsc]/[dirname.img]");
    settings.format(vfs_img);
    ASSERT_EQ(vfs_img, "resource/image");

    // get_formatted("vfs.img"):
    ASSERT_EQ(settings.get_formatted("vfs.vid"), "global_rsc/video");
}

int main(int argc, char** argv)
{
    program_dir = std::filesystem::canonical(*argv).parent_path().generic_string();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
