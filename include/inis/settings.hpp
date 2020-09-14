#pragma once

#include <filesystem>
#include <sstream>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <utility>
#include <string>
#include <string_view>
#include <cstdlib>

namespace inis
{
using namespace std::literals::string_literals;

template <typename value_type>
bool setting_string_to_value(const std::string& setting_value, value_type& value)
{
    std::istringstream stream(setting_value);
    if (stream >> value)
        return stream.eof();
    return false;
}

template <typename value_type>
std::string value_to_setting_string(const value_type& value)
{
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

class setting_value : public std::string
{
public:
    using std::string::string;

    setting_value(const std::string& str) : std::string(str) {}
    setting_value(std::string&& str) : std::string(std::move(str)) {}

    bool is_default() const { return empty() || *this == "[$default]"; }

    template <class value_type>
    value_type to(const value_type& default_value = value_type()) const
    {
        if(!is_default())
        {
            value_type res;
            if (setting_string_to_value(*this, res))
                return res;
        }
        return default_value;
    }
};

class section
{
    inline constexpr static std::string_view::value_type standard_label_mark_ = '$';

public:
    using settings_dictionnary = std::unordered_map<std::string, setting_value>;

    inline constexpr static std::string_view settings_dir = "$settings_dir";
    inline constexpr static std::string_view working_dir = "$working_dir";
    inline constexpr static std::string_view tmp_dir = "$tmp_dir";

    // constructors:
    section();
    explicit section(std::string name);

    // parent/root:
    inline section* parent() { return parent_; }
    inline const section* parent() const { return parent_; }
    section& root();
    const section& root() const;
    inline bool is_root() const { return parent_ == nullptr; }

    // name accessors:
    const std::string& name() const { return name_; }
    std::string& name() { return name_; }

    // settings accessors:
    inline const settings_dictionnary& settings() const { return settings_; }

    template <class value_type>
    requires (!(std::is_same_v<std::string, value_type> || std::is_same_v<std::string_view, value_type>))
    value_type setting(const std::string_view& setting_path, const value_type& default_value = value_type()) const
    {
        const setting_value* s_value = get_setting_value_ptr_(std::string(setting_path));
        if (s_value)
            return s_value->to<value_type>(default_value);
        return default_value;
    }

    template <class value_type>
    requires std::is_same_v<std::string, value_type>
    const std::string& setting(const std::string_view& setting_path, const std::string& default_value = std::string()) const
    {
        const setting_value* s_value = get_setting_value_ptr_(std::string(setting_path));
        if (s_value)
            return *s_value;
        return default_value;
    }

    template <class value_type>
    requires std::is_same_v<std::string_view, value_type>
    std::string_view setting(const std::string_view& setting_path, const std::string_view& default_value = std::string_view()) const
    {
        const setting_value* s_value = get_setting_value_ptr_(std::string(setting_path));
        if (s_value)
            return *s_value;
        return default_value;
    }

    template <class value_type>
    requires std::is_same_v<std::string_view, value_type>
    std::string_view setting(const std::string_view& setting_path, const std::string& default_value = std::string()) const
    {
        return setting<std::string>(setting_path, default_value);
    }

    // format:
    inline void format(std::string& var) const { format_(var, this); }

    std::string formatted_setting(const std::string_view& setting_path, const std::string& default_value = std::string()) const;

    // setting modifiers:
    bool set(const std::string& setting_path, const std::string& value);

    template <class value_type>
    bool set(const std::string& setting_path, const value_type& value)
    {
        setting_value* setting = get_setting_value_ptr_(setting_path);
        if (setting)
        {
            *setting = value_to_setting_string(value);
            return true;
        }
        return false;
    }

    // create subsections:
    section* create_sections(const std::string_view& section_path);

    // read/write file:
    void read_from_file(const std::filesystem::path& path);
    void write_to_file(const std::filesystem::path& path);

    // settings accessors:
    const section* child_ptr(const std::string& section_path) const;
    const section& child(const std::string& section_name) const { return *child_ptr(section_name); }
    section* child_ptr(const std::string& section_path);
    section& child(const std::string& section_name) { return *child_ptr(section_name); }

    // printing methods:
    void print_to_stream(std::ostream& stream, unsigned indent = 0) const;
    void print(unsigned indent = 0) const;

private:
    const setting_value* local_get_setting_value_ptr_(const std::string& setting_name) const;
    const setting_value* get_setting_value_ptr_(const std::string& setting_path) const;
    setting_value* get_setting_value_ptr_(const std::string& setting_path);
    void format_(std::string& var, const section *root) const;
    bool get_setting_value_if_exists_(const std::string& setting_path, std::string& value, const section *root) const;
    bool set_setting_from_line_(const std::string_view& line, setting_value*& current_value);
    bool extract_label_value_(std::string_view str, std::string_view& label, std::string_view& value, bool& value_is_multi_line);
    void append_line_to_current_value_(setting_value*& current_value, std::string line);
    void read_from_stream_(std::istream& stream, std::string& buffer);

    static std::string_view parent_section_path_(const std::string_view& path);
    static void split_setting_path_(const std::string_view& setting_path, std::string_view& section_path, std::string_view& setting);
    static void remove_comment(std::string_view& str);
    static void remove_spaces(std::string_view& str);
    static void remove_left_spaces(std::string_view& str);
    static void remove_right_spaces(std::string_view& str);

    section* parent_ = nullptr;
    std::string name_;
    settings_dictionnary settings_;
    std::unordered_map<std::string, std::unique_ptr<section>> sections_;
};

}
