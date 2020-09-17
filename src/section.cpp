#include <inis/inis.hpp>
#include <regex>
#include <fstream>
#include <iostream>

namespace inis
{
using namespace std::literals::string_literals;

template <class Char = char>
class Basic_string_tokenizer
{
public:
    using Value_type = Char;
    using String_view = std::basic_string_view<Value_type>;

private:
    using String_view_const_iterator = typename String_view::const_iterator;

public:
    Basic_string_tokenizer(String_view strv, const Char& sep = Char(' '))
    : str_(strv), current_(str_.begin()), sep_(sep)
    {}

    bool has_token() const
    {
        return current_ != str_.end();
    }

    String_view next_token()
    {
        std::size_t len = 0;
        String_view_const_iterator iter = current_;
        for (; iter != str_.end(); ++iter, ++len)
        {
            if (*iter == sep_)
            {
                ++iter;
                break;
            }
        }
        String_view tok(&*current_, len);
        current_ = std::move(iter);
        return tok;
    }

private:
    String_view str_;
    String_view_const_iterator current_;
    Char sep_;
};

using String_tokenizer = Basic_string_tokenizer<>;

//------------------------------------------------------------------------------

section::section()
{}

section::section(std::string name)
    : name_(std::move(name))
{}

section& section::root()
{
    section* root = this;
    while (root->parent_)
        root = root->parent_;
    return *root;
}

const section& section::root() const
{
    const section* root = this;
    while (root->parent_)
        root = root->parent_;
    return *root;
}

std::string section::formatted_setting(const std::string_view& setting_path, const std::string &default_value) const
{
    std::string value;
    std::string_view section_path;
    std::string_view setting_label;
    split_setting_path_(setting_path, section_path, setting_label);
    const section* section = subsection_ptr(std::string(section_path));
    if (!section) [[unlikely]]
        return default_value;
    const setting_value* s_value = section->local_get_setting_value_ptr_(std::string(setting_label));
    value = s_value ? *s_value : default_value;
    section->format_(value, this);
    return value;
}

const section* section::subsection_ptr(const std::string& section_path) const
{
    const section* settings = this;
    String_tokenizer tokenizer(section_path, '.');
    for (String_tokenizer::String_view token; tokenizer.has_token();)
    {
        token = tokenizer.next_token();
        auto iter = settings->sections_.find(std::string(token));
        if (iter == sections_.end())
            return nullptr;
        settings = iter->second.get();
    }
    return settings;
}

section* section::subsection_ptr(const std::string& section_path)
{
    section* settings = this;
    String_tokenizer tokenizer(section_path, '.');
    for (String_tokenizer::String_view token; tokenizer.has_token();)
    {
        token = tokenizer.next_token();
        auto iter = settings->sections_.find(std::string(token));
        if (iter == sections_.end())
            return nullptr;
        settings = iter->second.get();
    }
    return settings;
}

const setting_value* section::local_get_setting_value_ptr_(const std::string& setting_name) const
{
    auto iter = settings_.find(setting_name);
    return iter != settings_.end() ? &iter->second : nullptr;
}

const setting_value* section::get_setting_value_ptr_(const std::string& setting_path) const
{
    std::size_t index = setting_path.rfind('.');
    std::string section_name;
    if(index != std::string::npos && index > 0)
        section_name = setting_path.substr(0, index);

    const section* settings = this;
    if (!section_name.empty())
        settings = subsection_ptr(section_name);

    if (settings)
    {
        auto iter = settings->settings_.find(setting_path.substr(index+1));
        if (iter != settings->settings_.end())
            return &iter->second;
    }

    return nullptr;
}

setting_value* section::get_setting_value_ptr_(const std::string& setting_path)
{
    std::size_t index = setting_path.rfind('.');
    std::string section_name;
    if(index != std::string::npos && index > 0)
        section_name = setting_path.substr(0, index);

    section* settings = this;
    if (!section_name.empty())
        settings = subsection_ptr(section_name);

    if (settings)
    {
        auto iter = settings->settings_.find(setting_path.substr(index+1));
        if (iter != settings->settings_.end())
            return &iter->second;
    }

    return nullptr;
}

void section::format_(std::string &var, const section* root) const
{
    std::string regex_str = R"((\{()";
    regex_str += R"(\$)";
    regex_str += R"(?[\._[:alnum:]]+)\}))";
    std::regex var_regex(regex_str);

    auto reg_iter = std::sregex_iterator(var.begin(), var.end(), var_regex);
    auto reg_end_iter = std::sregex_iterator();

    std::smatch rmatch;
    std::string formatted_var;
    std::string::const_iterator bt_begin = var.begin();
    for (; reg_iter != reg_end_iter; ++reg_iter)
    {
        rmatch = *reg_iter;
        // get the matching token string
        auto matching_token = rmatch[1];
        // append the string token before the matching token to formatted_var
        formatted_var += std::string(bt_begin, matching_token.first);
        // set the begin before-token iterator to the end of the matching token
        bt_begin = matching_token.second;

        // get the setting path from the matching token string
        std::string setting_path = rmatch[2].str();
        // get the setting value if it exists among settings
        std::string new_str_token;
        if (!(get_setting_value_if_exists_(std::string(setting_path), new_str_token, root)))
            // if not, the string token to append to formatted_var is the entire matching token string
            new_str_token = matching_token.str();
        formatted_var += new_str_token;
    }
    // append the remaining string to formatted_var
    formatted_var += std::string(bt_begin, var.cend());
    var = std::move(formatted_var);
}

bool section::get_setting_value_if_exists_(const std::string& setting_path, std::string& value, const section* root) const
{
    const section* sec = this;
    std::string_view explicit_path = setting_path;
    resolve_implicit_path_part_(explicit_path, sec, root);
    std::string_view section_path;
    std::string_view setting_name;
    split_setting_path_(explicit_path, section_path, setting_name);
    sec = sec->subsection_ptr(std::string(section_path));

    const setting_value* s_value = sec->get_setting_value_ptr_(std::string(setting_name));
    if (s_value)
    {
        value = *s_value;
    }
    else
    {
        return false;
    }

    sec->format_(value, root);
    return true;
}

void section::write_to_stream_(std::ostream &stream, const section* const root, const std::string_view& default_value_end_marker)
{
    if (this != root)
    {
        stream << '[';
        if (parent_ != root)
            stream << '.';
        stream << name() << ']' << std::endl;
    }

    for (const auto& entry : settings_)
    {
        if (entry.first.front() == '$') [[unlikely]]
            continue;
        stream << entry.first;
        if (entry.second.find_first_of('\n') == std::string::npos)
            stream << " = " << entry.second << '\n';
        else
            stream << " =| " << default_value_end_marker << "\n"
                   << entry.second << "\n"
                   << default_value_end_marker << "\n";
    }
    if (!settings_.empty()) [[unlikely]]
        stream << '\n';

    for (const auto& entry : sections_)
        entry.second->write_to_stream_(stream, root, default_value_end_marker);
}

void section::resolve_implicit_path_part_(std::string_view& path, const section*& sec, const section* root)
{
    std::size_t offset = 0;
    const section* lroot = sec;
    for (auto iter = path.begin(); iter != path.end() && *iter == '.'; ++iter, ++offset)
    {
        if (!lroot || lroot == root)
            throw std::runtime_error(std::string("The section path is incorrect (Too many '.'): ") += path);
        lroot = lroot->parent();
    }
    for (; lroot && lroot != root; )
    {
        lroot = lroot->parent();
        sec = sec->parent();
    }
    path = path.substr(offset);
}

void section::resolve_implicit_path_part_(std::string_view& path, section*& sec, const section* root)
{
    std::size_t offset = 0;
    const section* lroot = sec;
    for (auto iter = path.begin(); iter != path.end() && *iter == '.'; ++iter, ++offset)
    {
        if (!lroot || lroot == root)
            throw std::runtime_error(std::string("The section path is incorrect (Too many '.'): ") += path);
        lroot = lroot->parent();
    }
    for (; lroot && lroot != root; )
    {
        lroot = lroot->parent();
        sec = sec->parent();
    }
    path = path.substr(offset);
}

bool section::set_setting(const std::string& setting_path, const std::string& value)
{
    std::regex label_regex(R"([\._[:alnum:]]+)"s);
    if (std::regex_match(setting_path, label_regex))
    {
        std::string_view section_path;
        std::string_view setting_name;
        split_setting_path_(setting_path, section_path, setting_name);
        section* sec = subsection_ptr(std::string(section_path));
        if (sec)
        {
            sec->settings_[setting_path] = value;
            return true;
        }
    }
    return false;
}

void section::read_from_stream(std::istream &stream)
{
    parser inis_parser(this);
    inis_parser.parse(stream);
}

void section::read_from_file(const std::filesystem::path& path)
{
    parser inis_parser(this);
    inis_parser.parse(path);
}

void section::write_to_stream(std::ostream &stream, std::string_view default_value_end_marker)
{
    write_to_stream_(stream, this, default_value_end_marker);
    stream.flush();
}

void section::write_to_file(const std::filesystem::path& path, std::string_view default_value_end_marker)
{
    std::ofstream stream(path);
    write_to_stream(stream, default_value_end_marker);
}

std::string_view section::parent_section_path_(const std::string_view& path)
{
    std::size_t index = path.rfind('.');
    if (index != std::string::npos && index > 0)
        return path.substr(0, index);
    return std::string_view();
}

void section::split_setting_path_(const std::string_view& setting_path, std::string_view& section_path, std::string_view& setting)
{
    std::size_t index = setting_path.rfind('.');
    if (index != std::string::npos && index > 0)
    {
        section_path = setting_path.substr(0, index);
        setting = setting_path.substr(index + 1);
    }
    else
    {
        section_path = std::string_view();
        setting = setting_path;
    }
}

section* section::create_sections(const std::string_view& section_path)
{
    if (std::regex_match(section_path.begin(), section_path.end(),
                         std::regex(R"(^([\._[:alnum:]]+)$)"s, std::regex_constants::ECMAScript)))
    {
        return create_sections_(section_path);
    }
    return nullptr;
}

section* section::create_sections_(const std::string_view& section_path)
{
    section* section_ptr = this;
    String_tokenizer tokenizer(section_path, '.');
    for (String_tokenizer::String_view token; tokenizer.has_token();)
    {
        token = tokenizer.next_token();
        auto& settings_uptr = section_ptr->sections_[std::string(token)];
        if (!settings_uptr)
        {
            settings_uptr = std::make_unique<section>(std::string(token));
            settings_uptr->parent_ = section_ptr;
        }
        section_ptr = settings_uptr.get();
    }

    return section_ptr;
}
//----------------------------------------------------------------
}
