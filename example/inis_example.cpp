#include <iostream>
#include <inis/inis.hpp>

int main()
{
    inis::section settings;
    inis::section* branch = settings.create_sections("[root.branch]");
    branch->set("key", "value");
    settings.print();
    std::cout << "EXIT SUCCESS" << std::endl;
    return EXIT_SUCCESS;
}
