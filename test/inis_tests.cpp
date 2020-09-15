#include <inis/settings.hpp>
#include <gtest/gtest.h>
#include <filesystem>
#include <cstdlib>

std::string program_dir;

TEST(inis_tests, basic_settings_test)
{
    std::filesystem::path inis_filepath("inis/basic_settings.inis");
    ASSERT_TRUE(std::filesystem::exists(inis_filepath));
    inis::section settings;
    settings.read_from_file(inis_filepath);

    // standard settings:
    ASSERT_EQ(inis::section::settings_dir, "$settings_dir");
    ASSERT_EQ(inis::section::working_dir, "$working_dir");
    ASSERT_EQ(inis::section::tmp_dir, "$tmp_dir");
    ASSERT_NE(settings.setting<std::string>(inis::section::settings_dir), "");
    ASSERT_NE(settings.setting<std::string>(inis::section::working_dir), "");
    ASSERT_NE(settings.setting<std::string>(inis::section::tmp_dir), "");

    // sections:
    ASSERT_EQ(settings.child_ptr(""), &settings);
    ASSERT_NE(settings.child_ptr("section"), nullptr);
    ASSERT_NE(settings.child_ptr("section.subsection"), nullptr);
    ASSERT_NE(settings.child_ptr("section.subsection2"), nullptr);
    ASSERT_EQ(settings.child_ptr("section.subsection.arg"), nullptr);
    ASSERT_EQ(settings.child_ptr("")->name(), "");
    ASSERT_EQ(settings.child_ptr("section")->name(), "section");
    ASSERT_EQ(settings.child_ptr("section.subsection")->name(), "subsection");
    ASSERT_EQ(settings.child_ptr("section.subsection2")->name(), "subsection2");

    // user settings:
    ASSERT_EQ(settings.setting<std::string>("global_label"), "value");
    ASSERT_EQ(settings.setting<std::string>("section.level"), "0");
    ASSERT_EQ(settings.setting<int>("section.level"), 0);
    ASSERT_EQ(settings.setting<std::string>("section.arg"), "Text on\nseveral lines.");
    ASSERT_EQ(settings.setting<std::string>("section.text"), "Begin of the text...\n\n... end of the text.");
    ASSERT_EQ(settings.setting<std::string>("section.failed_arg"), "|");
    ASSERT_EQ(settings.setting<std::string>("section.splitted"), "A single line written on two lines in the file.");
    ASSERT_EQ(settings.setting<std::string>("section.subsection.arg"), "45.5");
    ASSERT_DOUBLE_EQ(settings.setting<double>("section.subsection.arg"), 45.5);
    ASSERT_EQ(settings.setting<std::string>("section.subsection2.arg"), "46.5");
    ASSERT_DOUBLE_EQ(settings.setting<double>("section.subsection2.arg"), 46.5);
    ASSERT_EQ(settings.setting<std::string>("bad_int"), "45a");
    ASSERT_EQ(settings.setting<int>("bad_int", -1), -1);
//    ASSERT_EQ(settings.get<std::string>("section.subsection2.last_label", "err"), "err");
//    ASSERT_EQ(settings.get<std::string>("last_label"), "end");
}

TEST(inis_tests, format_test)
{
    std::filesystem::path inis_filepath("inis/settings.inis");
    ASSERT_TRUE(std::filesystem::exists(inis_filepath));
    inis::section settings;
    settings.read_from_file(inis_filepath);

    // comment:
    std::string value = settings.setting<std::string>("comment");
    ASSERT_EQ(value, "{Version: '{version}'}");
    settings.format(value);
    ASSERT_EQ(value, "{Version: '0.1.0'}");

    // get_formatted("vfs.img"):
    ASSERT_EQ(settings.formatted_setting("vfs.img"), "resource/image");

    // get<std::string>("vfs.img"), format(vfs_img):
    std::string vfs_img = settings.setting<std::string>("vfs.img");
    ASSERT_EQ(vfs_img, "{vfs.rsc}/{dirname.img}");
    settings.format(vfs_img);
    ASSERT_EQ(vfs_img, "resource/image");

    // get_formatted("vfs.img"):
//    std::cout << "formatted_setting implicit_path" << std::endl;
    ASSERT_EQ(settings.formatted_setting("vfs.vid"), "resource/video");
    ASSERT_EQ(settings.formatted_setting("root.branch.leaf.key_2"), "value_2");
    ASSERT_EQ(settings.formatted_setting("root.branch.leaf.key_2_3"), "value_2_3");
    ASSERT_EQ(settings.formatted_setting("root.branch.leaf.special"), "value_2resource/video");
}

int main(int argc, char** argv)
{
    program_dir = std::filesystem::canonical(*argv).parent_path().generic_string();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
