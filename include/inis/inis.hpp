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

    bool is_default() const { return empty(); }

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

    friend class parser;
    class parser
    {
        enum value_category : uint8_t
        {
            Single_line = 0,
            Multi_line = 1,
            Split_line = 2,
        };

        parser(section* section, const std::string_view& comment_marker);

    public:
        explicit parser(section* section);
        inline const section* sec() const { return this_section_; }
        inline section* sec() { return this_section_; }
        inline const std::string_view& comment_marker() const { return comment_marker_; }
        void parse(std::istream& stream);
        void parse(const std::filesystem::path &setting_filepath);

    private:
        void read_from_stream_(std::istream& stream);
        bool try_create_setting_(const std::string_view& line);
        bool try_create_sections_(const std::string_view& line);
        void append_line_to_current_value_(const std::string_view &line);
        void reset_current_value_status_();
        static bool extract_name_and_value_(std::string_view str, std::string_view& label,
                                            std::string_view& value, std::string_view &value_end_marker, value_category &value_cat);
        void remove_comment_(std::string_view& str);
        static void remove_spaces_(std::string_view& str);
        static void remove_left_spaces_(std::string_view& str);
        static void remove_right_spaces_(std::string_view& str);

    private:
        section* this_section_;
        std::string_view comment_marker_;
        // current status:
        section* current_section_;
        setting_value* current_value_;
        value_category current_value_category_;
        std::string current_value_end_marker_;
    };

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

    template <class value_type, class default_value_type>
    requires std::is_same_v<std::string, value_type>
    && (std::is_same_v<default_value_type, std::string_view>
        || std::is_array_v<default_value_type>)
    std::string setting(const std::string_view& setting_path, const default_value_type& default_value) const
    {
        const setting_value* s_value = get_setting_value_ptr_(std::string(setting_path));
        if (s_value && !s_value->is_default())
            return *s_value;
        return std::string(default_value);
    }

    template <class value_type>
    requires std::is_same_v<std::string, value_type>
    const std::string& setting(const std::string_view& setting_path, const std::string& default_value) const
    {
        const setting_value* s_value = get_setting_value_ptr_(std::string(setting_path));
        if (s_value && !s_value->is_default())
            return *s_value;
        return default_value;
    }

    template <class value_type>
    requires std::is_same_v<std::string, value_type>
    std::string setting(const std::string_view& setting_path, std::string&& default_value) const
    {
        const setting_value* s_value = get_setting_value_ptr_(std::string(setting_path));
        if (s_value && !s_value->is_default())
            return *s_value;
        return std::string(default_value);
    }

    template <class value_type>
    requires std::is_same_v<std::string, value_type>
    std::string setting(const std::string_view& setting_path) const
    {
        const setting_value* s_value = get_setting_value_ptr_(std::string(setting_path));
        if (s_value)
            return *s_value;
        return std::string();
    }

    template <class value_type>
    requires std::is_same_v<std::string_view, value_type>
    std::string_view setting(const std::string_view& setting_path, const std::string_view& default_value = std::string_view()) const
    {
        const setting_value* s_value = get_setting_value_ptr_(std::string(setting_path));
        if (s_value && !s_value->is_default())
            return *s_value;
        return default_value;
    }

    // format:
    inline void format(std::string& var) const { format_(var, this); }

    std::string formatted_setting(const std::string_view& setting_path, const std::string& default_value = std::string()) const;

    // setting modifiers:
    bool set_setting(const std::string& setting_path, const std::string& value);

    template <class value_type>
    requires (!std::is_same_v<value_type, std::string>)
    bool set_setting(const std::string& setting_path, const value_type& value)
    {
        return set_setting(setting_path, value_to_setting_string(value));
    }

    // create subsections:
    section* create_sections(const std::string_view& section_path);

    // read:
    void read_from_stream(std::istream& stream);
    void read_from_file(const std::filesystem::path& path);
    // write:
    void write_to_stream(std::ostream& stream, std::string_view default_value_end_marker = "");
    void write_to_file(const std::filesystem::path& path, std::string_view default_value_end_marker);

    // settings accessors:
    const section* subsection_ptr(const std::string& section_path) const;
    inline const section& subsection(const std::string& section_name) const { return *subsection_ptr(section_name); }
    section* subsection_ptr(const std::string& section_path);
    inline section& subsection(const std::string& section_name) { return *subsection_ptr(section_name); }

private:
    section* create_sections_(const std::string_view& section_path);
    const setting_value* local_get_setting_value_ptr_(const std::string& setting_name) const;
    const setting_value* get_setting_value_ptr_(const std::string& setting_path) const;
    setting_value* get_setting_value_ptr_(const std::string& setting_path);
    void format_(std::string& var, const section *root) const;
    bool get_setting_value_if_exists_(const std::string& setting_path, std::string& value, const section *root) const;
    void write_to_stream_(std::ostream& stream, const section * const root, const std::string_view &default_value_end_marker);
    static void resolve_implicit_path_part_(std::string_view &path, const section*& section, const class section* root);
    static void resolve_implicit_path_part_(std::string_view &path, section*& sec, const section* root);
    static std::string_view parent_section_path_(const std::string_view& path);
    static void split_setting_path_(const std::string_view& setting_path, std::string_view& section_path, std::string_view& setting);

private:
    section* parent_ = nullptr;
    std::string name_;
    settings_dictionnary settings_;
    std::unordered_map<std::string, std::unique_ptr<section>> sections_;
};

}
