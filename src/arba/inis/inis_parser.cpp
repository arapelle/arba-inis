#include <arba/inis/inis.hpp>

#include <fstream>
#include <iostream>
#include <regex>
#include <string_view>

inline namespace arba
{
namespace inis
{

using namespace std::literals::string_literals;

section::parser::parser(section* section, const std::string_view& comment_marker)
    : this_section_(section), comment_marker_(comment_marker), current_section_(nullptr), current_value_(nullptr),
      current_value_category_(Single_line)
{
}

section::parser::parser(section* section) : parser(section, std::string_view("//"))
{
}

void section::parser::parse(std::istream& stream)
{
    read_from_stream_(stream);
}

void section::parser::parse(const std::filesystem::path& setting_filepath)
{
    this_section_->settings_.insert_or_assign(
        std::string(settings_dir), std::filesystem::canonical(setting_filepath).parent_path().generic_string());
    std::ifstream stream(setting_filepath);
    read_from_stream_(stream);
}

void section::parser::read_from_stream_(std::istream& stream)
{
    if (this_section_->is_root())
    {
        this_section_->settings_.insert_or_assign(
            std::string(working_dir), std::filesystem::canonical(std::filesystem::current_path()).generic_string());
        this_section_->settings_.insert_or_assign(std::string(tmp_dir),
                                                  std::filesystem::temp_directory_path().generic_string());
        //        section_->settings_.insert_or_assign("$program_dir"s, "???");
    }
    else
    {
        std::cerr << "WARNING: Load from a section node which is not root." << std::endl;
    }

    current_section_ = this_section_;
    current_value_ = nullptr;

    std::string buffer;
    buffer.reserve(120);
    while (stream && !stream.eof())
    {
        std::getline(stream, buffer);
        std::string_view line(buffer);
        remove_comment_(line);
        remove_right_spaces_(line);

        if (try_create_setting_(line))
            continue;

        if (try_create_sections_(line))
            continue;

        if (current_value_)
        {
            append_line_to_current_value_(line);
            continue;
        }

        if (!line.empty())
            std::cerr << "WARNING: Bad line : '" << line << "'" << std::endl;
    }
}

bool section::parser::try_create_setting_(const std::string_view& line)
{
    std::string_view label;
    std::string_view value;
    std::string_view value_end_marker;
    value_category value_cat = Single_line;
    if (extract_name_and_value_(line, label, value, value_end_marker, value_cat))
    {
        settings_dictionnary::value_type setting(label, value);
        auto insert_res = current_section_->settings_.emplace(std::move(setting));
        current_value_category_ = value_cat;
        if (value_cat != Single_line)
        {
            current_value_ = &insert_res.first->second;
            current_value_end_marker_ = value_end_marker;
        }
        else
            reset_current_value_status_();
        return true;
    }
    return false;
}

bool section::parser::try_create_sections_(const std::string_view& line)
{
    std::match_results<std::string_view::const_iterator> match;
    if (std::regex_match(line.begin(), line.end(), match,
                         std::regex(R"(^\[([\._[:alnum:]]+)\]$)"s, std::regex_constants::ECMAScript)))
    {
        const auto& sm = match[1];
        std::string_view section_path = std::string_view(sm.first, sm.length());

        section* sec = current_section_ ? current_section_ : this_section_;
        std::string_view explicit_path = section_path;
        resolve_implicit_path_part_(explicit_path, sec, this_section_);

        if (section* leaf_section = sec->create_sections_(explicit_path))
        {
            current_section_ = leaf_section;
            reset_current_value_status_();
            return true;
        }
    }
    return false;
}

void section::parser::append_line_to_current_value_(const std::string_view& line)
{
    if (line != current_value_end_marker_)
    {
        if (!current_value_->empty())
        {
            switch (current_value_category_)
            {
            case Multi_line:
                current_value_->reserve(current_value_->length() + line.length() + 1);
                current_value_->append(1, '\n');
                break;
            case Split_line:
                current_value_->reserve(current_value_->length() + line.length());
                break;
            default:
                std::cerr << "ERROR: Value category not handled! " << current_value_ << std::endl;
            }
            current_value_->append(std::move(line));
        }
        else
            *current_value_ = std::string(line);
    }
    else
    {
        reset_current_value_status_();
    }
}

void section::parser::reset_current_value_status_()
{
    current_value_ = nullptr;
    current_value_category_ = Single_line;
    current_value_end_marker_.clear();
}

bool section::parser::extract_name_and_value_(std::string_view str, std::string_view& label, std::string_view& value,
                                              std::string_view& value_end_marker, value_category& value_cat)
{
    std::size_t index = str.find('=');
    if (index != std::string::npos)
    {
        label = str.substr(0, index);
        remove_spaces_(label);
        value = str.substr(index + 1);

        if (!value.empty())
        {
            switch (value.front())
            {
            case '|':
                value_cat = Multi_line;
                value_end_marker = value.substr(1);
                remove_spaces_(value_end_marker);
                break;
            case '>':
                value_cat = Split_line;
                break;
            default:
                value_cat = Single_line;
                remove_spaces_(value);
            }
            if (value_cat != Single_line)
                value = std::string_view();
        }
        return true;
    }
    return false;
}

void section::parser::remove_comment_(std::string_view& str)
{
    std::size_t index = str.find(comment_marker_);
    if (index != std::string::npos)
        str.remove_suffix(str.length() - index);
}

void section::parser::remove_spaces_(std::string_view& str)
{
    remove_right_spaces_(str);
    remove_left_spaces_(str);
}

void section::parser::remove_left_spaces_(std::string_view& str)
{
    auto iter = std::find_if(str.begin(), str.end(), std::not_fn(isspace));
    if (iter != str.end())
        str.remove_prefix(iter - str.begin());
}

void section::parser::remove_right_spaces_(std::string_view& str)
{
    auto riter = std::find_if(str.rbegin(), str.rend(), std::not_fn(isspace));
    if (riter != str.rend())
        str.remove_suffix(str.end() - riter.base());
}

} // namespace inis
} // namespace arba
