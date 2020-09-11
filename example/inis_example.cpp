#include <iostream>
#include <inis/settings.hpp>

int main()
{
    inis::settings settings;
    inis::settings* branch = settings.create_sections("[root.branch]");
    branch->set("key", "value");
    settings.print();
    std::cout << "EXIT SUCCESS" << std::endl;
    return EXIT_SUCCESS;
}
