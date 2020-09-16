#include <iostream>
#include <inis/inis.hpp>

int main()
{
    inis::section settings;
    inis::section* branch = settings.create_sections("root.branch");
    branch->set_setting("key", "value");
    settings.write_to_stream(std::cout);
    std::cout << "EXIT SUCCESS" << std::endl;
    return EXIT_SUCCESS;
}
