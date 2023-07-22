# Concept

The purpose is to read/write settings formatted in *inis* files, in C++. *inis* files are like [*ini*](https://en.wikipedia.org/wiki/INI_file) file, with slight differences:

- Comments begin with `//`.
- If `[section]`is the last declared section, declaring `[.subsection]` is equivalent to `[section.subsection]`.

# Install

## Requirements

Binaries:

- A C++20 compiler (ex: g++-13)
- CMake 3.26 or later

Testing Libraries (optional):
- [Google Test](https://github.com/google/googletest) 1.13 or later (optional)

## Clone

```
git clone https://github.com/arapelle/arba-inis --recurse-submodules
```

## Quick Install

There is a cmake script at the root of the project which builds the library in *Release* mode and install it (default options are used).

```
cd /path/to/arba-inis
cmake -P cmake/scripts/quick_install.cmake
```

Use the following to quickly install a different mode.

```
cmake -P cmake/scripts/quick_install.cmake -- TESTS BUILD Debug DIR /tmp/local
```

## Uninstall

There is a uninstall cmake script created during installation. You can use it to uninstall properly this library.

```
cd /path/to/installed-arba-inis/
cmake -P uninstall.cmake
```

# How to use

## Example - Create *inis* settings

```c++
#include <iostream>
#include <arba/inis/inis.hpp>

int main()
{
    inis::section settings;
    inis::section* branch = settings.create_sections("root.branch");
    branch->set_setting("key", "value");
    settings.write_to_stream(std::cout);
    std::cout << "EXIT SUCCESS" << std::endl;
    return EXIT_SUCCESS;
}
```

## Example - Read *inis* settings

```c++
#include <iostream>
#include <arba/inis/inis.hpp>

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

    std::cout << "EXIT SUCCESS" << std::endl;
    return EXIT_SUCCESS;
}
```

## Example - Using *inis* in a CMake project

See the *basic_cmake_project* example, and more specifically the *CMakeLists.txt* to see how to use *inis* in your CMake projects.

# License

[MIT License](./LICENSE.md) © arba-inis
