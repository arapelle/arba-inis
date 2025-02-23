#include <arba/inis/inis.hpp>

#include <iostream>

int main()
{
    std::string inis_str = R"inis(
key = value
[section]
[.subsection]
  number = 42
  str = My Super string!
)inis";
    std::istringstream stream(inis_str);

    inis::section settings;
    settings.read_from_stream(stream);
    std::cout << "----------------" << std::endl;
    settings.write_to_stream(std::cout);
    std::cout << "----------------" << std::endl;
    int num = settings.setting<int>("section.subsection.number");
    std::string text = settings.setting<std::string>("section.subsection.str");
    std::cout << num << std::endl;
    std::cout << text << std::endl;

    std::cout << "TEST PACKAGE SUCCESS" << std::endl;
    return EXIT_SUCCESS;
}
