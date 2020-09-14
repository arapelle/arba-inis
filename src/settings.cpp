#include <inis/settings.hpp>
#include <fstream>
#include <iostream>

namespace inis
{
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

settings& settings::root()
{
    settings* root = this;
    while (root->parent_)
        root = root->parent_;
    return *root;
}

const settings& settings::root() const
{
    const settings* root = this;
    while (root->parent_)
        root = root->parent_;
    return *root;
}

std::string settings::get_formatted(const std::string_view& setting_path, const std::string &default_value) const
{
    std::string value;
    std::string_view section_path;
    std::string_view setting_label;
    split_setting_path_(setting_path, section_path, setting_label);
    const settings* settings = section_ptr(std::string(section_path));
    if (!settings) [[unlikely]]
        return default_value;
    const setting_value* s_value = settings->local_get_setting_value_ptr_(std::string(setting_label));
    value = s_value ? *s_value : default_value;
    /*settings->*/format_(value, this);
    return value;
}

const settings* settings::section_ptr(const std::string& section_path) const
{
    const settings* settings = this;
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

const setting_value* settings::local_get_setting_value_ptr_(const std::string& label) const
{
    auto iter = settings_.find(label);
    if (iter != settings_.end())
    {
        return &iter->second;
    }
    return nullptr;
}

const setting_value* settings::get_setting_value_ptr_(const std::string& label) const
{
    std::size_t index = label.rfind('.');
    std::string section_name;
    if(index != std::string::npos && index > 0)
        section_name = label.substr(0, index);

    const settings* settings = this;
    if (!section_name.empty())
        settings = section_ptr(section_name);

    if (settings)
    {
        auto iter = settings->settings_.find(label.substr(index+1));
        if (iter != settings->settings_.end())
            return &iter->second;
    }

    return nullptr;
}

void settings::format_(std::string &var, const settings* root) const
{
    std::string regex_str = R"((\[()";
    regex_str += R"(\$)";
    regex_str += R"(?[\._[:alnum:]]+)\]))";
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

bool settings::get_setting_value_if_exists_(const std::string& setting_path, std::string& value, const settings* root) const
{
    const settings* settings = this;

    for (;;)
    {
        const setting_value* ptr = settings->get_setting_value_ptr_(setting_path);
        if (ptr)
        {
            value = *ptr;
            break;
        }
        else if (settings != root)
        {
            settings = settings->parent_;
        }
        else
        {
            return false;
        }
    }

    settings->format(value);
    return true;
}

bool settings::set_setting_from_line_(const std::string_view& line, setting_value*& current_value)
{
    std::string_view label;
    std::string_view value;
    bool value_is_multi_line = false;
    if (extract_label_value_(line, label, value, value_is_multi_line))
    {
        settings_dict::value_type setting(label, value);
        auto eres = settings_.emplace(std::move(setting));
        if (value_is_multi_line)
            current_value = &eres.first->second;
        else
            current_value = nullptr;
        return true;
    }
    return false;
}

bool settings::extract_label_value_(std::string_view str, std::string_view& label, std::string_view& value, bool& value_is_multi_line)
{
    std::size_t index = str.find('=');
    if(index != std::string::npos)
    {
        label = str.substr(0, index);
        remove_spaces(label);
        value = str.substr(index + 1);
        value_is_multi_line = !value.empty() && value.front() == '|';
        if (value_is_multi_line)
            value = std::string_view();
        else
            remove_spaces(value);
        return true;
    }
    return false;
}

void settings::append_line_to_current_value_(setting_value*& current_value, std::string line)
{
    if (!line.empty())
    {
        if (!current_value->empty())
        {
            current_value->reserve(current_value->length() + line.length() + 1);
            current_value->append(1, '\n');
            current_value->append(std::move(line));
        }
        else
            *current_value = std::move(line);
    }
    else
        current_value = nullptr;
}

bool settings::set(const std::string& label, const std::string& value)
{
    std::regex label_regex("[_[:alnum:]]+"s);
    if (std::regex_match(label, label_regex))
    {
        settings_[label] = value;
        return true;
    }
    return false;
}

void settings::load_from_file(const std::filesystem::path& path)
{
    settings_.insert_or_assign(std::string(settings_dir),
                               std::filesystem::canonical(path).parent_path().generic_string());
    if (is_root())
    {
        settings_.insert_or_assign(std::string(working_dir),
                                   std::filesystem::canonical(std::filesystem::current_path()).generic_string());
        settings_.insert_or_assign(std::string(tmp_dir),
                                   std::filesystem::temp_directory_path().generic_string());
//        settings_.insert_or_assign("<program_dir>"s, "???");
    }
    else
    {
        std::cout << "WARNING: load from a settings node which is not root." << std::endl;
    }

    std::ifstream stream(path.string());
    std::string buffer;
    load_from_stream_(stream, buffer);
}

void settings::save_to_file(const std::filesystem::path& path)
{
    std::ofstream stream(path.string());
    write_to_stream(stream);
}

void settings::load_from_stream_(std::istream& stream, std::string& buffer)
{
    settings* settings_ptr = this;
    setting_value* current_value = nullptr;

    while (stream && !stream.eof())
    {
        std::getline(stream, buffer);
        std::string_view line(buffer);
        remove_comment(line);
        remove_right_spaces(line);

        if (settings_ptr->set_setting_from_line_(line, current_value))
            continue;

        if (settings* leaf_settings = this->create_sections(line))
        {
            settings_ptr = leaf_settings;
            current_value = nullptr;
            continue;
        }

        if (current_value)
        {
            append_line_to_current_value_(current_value, std::string(line));
            continue;
        }

        if (!line.empty())
            std::cerr << "BAD LINE : '" << line << "'" << std::endl;
    }
}

std::string_view settings::parent_section_path_(const std::string_view& path)
{
    std::size_t index = path.rfind('.');
    if (index != std::string::npos && index > 0)
        return path.substr(0, index);
    return std::string_view();
}

void settings::split_setting_path_(const std::string_view& setting_path, std::string_view& section_path, std::string_view& setting)
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



settings* settings::create_sections(const std::string_view& section_path)
{
    std::match_results<std::string_view::const_iterator> match;
    if(std::regex_match(section_path.begin(), section_path.end(), match, std::regex(R"(^\[([\._[:alnum:]]+)\]$)"s, std::regex_constants::ECMAScript)))
    {
        const auto& sm = match[1];
        std::string_view section_name = std::string_view(sm.first, sm.length());

        settings* settings_ptr = this;
        String_tokenizer tokenizer(section_name, '.');
        for (String_tokenizer::String_view token; tokenizer.has_token();)
        {
            token = tokenizer.next_token();
            auto& settings_uptr = settings_ptr->sections_[std::string(token)];
            if (!settings_uptr)
            {
                settings_uptr = std::make_unique<settings>();
                settings_uptr->parent_ = settings_ptr;
            }
            settings_ptr = settings_uptr.get();
        }

        return settings_ptr;
    }
    return nullptr;
}

void settings::remove_comment(std::string_view& str)
{
    std::size_t index = str.find('#');
    if(index != std::string::npos)
        str.remove_suffix(str.length() - index);
}

void settings::remove_spaces(std::string_view& str)
{
    remove_right_spaces(str);
    remove_left_spaces(str);
}

void settings::remove_left_spaces(std::string_view& str)
{
    auto iter = std::find_if(str.begin(), str.end(), std::not_fn(isspace));
    if(iter != str.end())
        str.remove_prefix(iter - str.begin());
}

void settings::remove_right_spaces(std::string_view& str)
{
    auto riter = std::find_if(str.rbegin(), str.rend(), std::not_fn(isspace));
    if(riter != str.rend())
        str.remove_suffix(str.end() - riter.base());
}

void settings::write_to_stream(std::ostream& stream, unsigned indent) const
{
    for (const auto& entry : settings_)
    {
        for (unsigned i = indent; i > 0; --i)
            stream << "  ";
        if (entry.first.front() != standard_label_mark_)
            stream << entry.first << " = " << entry.second << std::endl;
    }

    for (const auto& entry : sections_)
    {
        for (unsigned i = indent; i > 0; --i)
            stream << "  ";
        stream << "[" << entry.first << "]" << std::endl;
        entry.second->write_to_stream(stream, indent + 1);
    }
}

void settings::print(unsigned indent) const
{
    write_to_stream(std::cout, indent);
}
}
