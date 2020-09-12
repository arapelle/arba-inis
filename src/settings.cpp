#include <inis/settings.hpp>
#include <string_view>
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

const settings* settings::section_ptr(const std::string& section_name) const
{
    const settings* settings = this;
    String_tokenizer tokenizer(section_name, '.');
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

void settings::format(std::string& var)
{
    const settings& root_settings = root();

    std::smatch rmatch;
    std::string regex_str = R"((\[()";
    regex_str += R"(\$)";
    regex_str += R"(?[\._[:alnum:]]+)\]))";
    std::regex var_regex(regex_str);

    auto r_iter = std::sregex_iterator(var.begin(), var.end(), var_regex);
    auto r_end_iter = std::sregex_iterator();

    std::string new_var;
    std::string::const_iterator bbegin = var.begin();
    for (; r_iter != r_end_iter; ++r_iter)
    {
        rmatch = *r_iter;
        auto mm = rmatch[1];
        new_var += std::string(bbegin, mm.first);
        bbegin = mm.second;
        std::string name = rmatch[2].str();
        auto iter = settings_.find(name);
        if (iter != settings_.end())
            new_var += iter->second;
        else
        {
            const setting_value* setting = root_settings.get_setting_value_ptr_(name);
            if (setting)
            {
                std::string value_str = *setting;
                format(value_str);
                new_var += value_str;
            }
            else
                new_var += mm.str();
        }
    }
    new_var += std::string(bbegin, var.cend());
    var = new_var;
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
