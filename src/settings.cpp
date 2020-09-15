#include <inis/settings.hpp>
#include <regex>
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
    const section* section = child_ptr(std::string(section_path));
    if (!section) [[unlikely]]
        return default_value;
    const setting_value* s_value = section->local_get_setting_value_ptr_(std::string(setting_label));
    value = s_value ? *s_value : default_value;
    section->format_(value, this);
    return value;
}

const section* section::child_ptr(const std::string& section_path) const
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

section* section::child_ptr(const std::string& section_path)
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
        settings = child_ptr(section_name);

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
        settings = child_ptr(section_name);

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
    std::size_t offset = find_explicit_path_start_(setting_path, sec, root);
    std::string explicit_path = setting_path.substr(offset);
    std::string_view section_path;
    std::string_view setting_name;
    split_setting_path_(explicit_path, section_path, setting_name);
    sec = sec->child_ptr(std::string(section_path));

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

std::size_t section::find_explicit_path_start_(const std::string_view& setting_path, const section*& sec, const section* root) const
{
    std::size_t offset = 0;
    const section* lroot = sec;
    for (auto iter = setting_path.begin(); iter != setting_path.end() && *iter == '.'; ++iter, ++offset)
        lroot = lroot->parent();
    for (; lroot && lroot != root; )
    {
        lroot = lroot->parent();
        sec = sec->parent();
    }
    return offset;
}

bool section::set(const std::string& setting_path, const std::string& value)
{
    std::regex label_regex("[_[:alnum:]]+"s);
    if (std::regex_match(setting_path, label_regex))
    {
        settings_[setting_path] = value;
        return true;
    }
    return false;
}

void section::read_from_file(const std::filesystem::path& path)
{
    settings_.insert_or_assign(std::string(settings_dir),
                               std::filesystem::canonical(path).parent_path().generic_string());
    std::ifstream stream(path.string());
    parser inis_parser(this, stream, path);
    inis_parser.parse();
}

void section::write_to_file(const std::filesystem::path& path)
{
    std::ofstream stream(path.string());
    print_to_stream(stream);
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
    std::match_results<std::string_view::const_iterator> match;
    if(std::regex_match(section_path.begin(), section_path.end(), match, std::regex(R"(^\[([\._[:alnum:]]+)\]$)"s, std::regex_constants::ECMAScript)))
    {
        const auto& sm = match[1];
        std::string_view section_name = std::string_view(sm.first, sm.length());

        section* section_ptr = this;
        String_tokenizer tokenizer(section_name, '.');
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
    return nullptr;
}

void section::print_to_stream(std::ostream& stream, unsigned indent) const
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
        entry.second->print_to_stream(stream, indent + 1);
    }
}

void section::print(unsigned indent) const
{
    print_to_stream(std::cout, indent);
}

//----------------------------------------------------------------


section::parser::parser(section* section, std::istream& stream, std::string_view comment_marker)
    : this_section_(section), stream_(stream), comment_marker_(comment_marker), current_section_(nullptr), current_value_(nullptr)
{}

section::parser::parser(section *section, std::istream& stream, const std::filesystem::path& setting_filepath)
    : parser(section, stream, deduce_comment_marker_from_(setting_filepath))
{}

void section::parser::parse()
{
    std::string buffer;
    buffer.reserve(120);
    read_from_stream_(buffer);
}

void section::parser::read_from_stream_(std::string& buffer)
{
    if (this_section_->is_root())
    {
        this_section_->settings_.insert_or_assign(std::string(working_dir),
                                   std::filesystem::canonical(std::filesystem::current_path()).generic_string());
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

    while (stream_ && !stream_.eof())
    {
        std::getline(stream_, buffer);
        std::string_view line(buffer);
        remove_comment_(line);
        remove_right_spaces_(line);

        if (create_setting_(line))
            continue;

        if (section* leaf_section = create_sections_(line))
        {
            current_section_ = leaf_section;
            current_value_ = nullptr;
            continue;
        }

        if (current_value_)
        {
            append_line_to_current_value_(line);
            continue;
        }

        if (!line.empty())
            std::cerr << "WARNING: Bad line : '" << line << "'" << std::endl;
    }
}

bool section::parser::create_setting_(const std::string_view &line)
{
    std::string_view label;
    std::string_view value;
    bool value_is_multi_line = false;
    if (extract_name_and_value_(line, label, value, value_is_multi_line))
    {
        settings_dictionnary::value_type setting(label, value);
        auto insert_res = current_section_->settings_.emplace(std::move(setting));
        if (value_is_multi_line)
            current_value_ = &insert_res.first->second;
        else
            current_value_ = nullptr;
        return true;
    }
    return false;
}

section *section::parser::create_sections_(const std::string_view &section_path) // ok
{
    return this_section_->create_sections(section_path);
}

void section::parser::append_line_to_current_value_(const std::string_view& line) // ok
{
    if (!line.empty())
    {
        if (!current_value_->empty())
        {
            current_value_->reserve(current_value_->length() + line.length() + 1);
            current_value_->append(1, '\n');
            current_value_->append(std::move(line));
        }
        else
            *current_value_ = std::string(line);
    }
    else
        current_value_ = nullptr;
}

std::string_view section::parser::deduce_comment_marker_from_(const std::filesystem::path& setting_filepath)
{
    std::string ext = setting_filepath.extension().generic_string();
    if (ext == ".inis")
        return "//";
    return ";";
}

bool section::parser::extract_name_and_value_(std::string_view str, std::string_view &label,
                                           std::string_view &value, bool &value_is_multi_line)
{
    std::size_t index = str.find('=');
    if (index != std::string::npos)
    {
        label = str.substr(0, index);
        remove_spaces_(label);
        value = str.substr(index + 1);
        value_is_multi_line = !value.empty() && value.front() == '|';
        if (value_is_multi_line)
            value = std::string_view();
        else
            remove_spaces_(value);
        return true;
    }
    return false;
}

void section::parser::remove_comment_(std::string_view& str)
{
    std::size_t index = str.find(comment_marker_);
    if(index != std::string::npos)
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
    if(iter != str.end())
        str.remove_prefix(iter - str.begin());
}

void section::parser::remove_right_spaces_(std::string_view& str)
{
    auto riter = std::find_if(str.rbegin(), str.rend(), std::not_fn(isspace));
    if(riter != str.rend())
        str.remove_suffix(str.end() - riter.base());
}

}
