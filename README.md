# Not Enough Standards

Not Enough Standards is a modern header-only C++17 and C++20 library that provides platform-independent utilities. The goal of this library is to extend the standard library with recurent features, such as process management, shared library loading or thread pools. To reach that goal the library is written in a very standard compliant way, from the coding-style to the naming convention.

## Features

Not Enough Standards works on any posix-compliant system and also on Windows.

* [Shared library loading](https://github.com/Alairion/not-enough-standards/wiki/shared_library.hpp)
* [Process management](https://github.com/Alairion/not-enough-standards/wiki/process.hpp)
* Inter-process communication ([pipes](https://github.com/Alairion/not-enough-standards/wiki/pipe.hpp), [shared memory](https://github.com/Alairion/not-enough-standards/wiki/shared_memory.hpp))
* Inter-process synchronization ([named mutexes](https://github.com/Alairion/not-enough-standards/wiki/named_mutex.hpp), [named semaphores](https://github.com/Alairion/not-enough-standards/wiki/names_semaphore.hpp))
* Synchronization primitives ([semaphores](https://github.com/Alairion/not-enough-standards/wiki/semaphore.hpp))
* [Thread pools](https://github.com/Alairion/not-enough-standards/wiki/thread_pool.hpp)

Check out the [Wiki](https://github.com/Alairion/not-enough-standards/wiki) for more informations.

## Installation

Not Enough Standards requires a C++17 compiler, and a C++20 compiler for thread pools.

As any header only library, Not Enough Standards is designed to be directly included in your project, by copying the files you need in your project's directory.

You may also use it as a CMake subproject using `add_subdirectory`, and use it as any other library:
```
target_link_libraries(xxx NotEnoughStandards)
target_include_directories(xxx PRIVATE ${NES_INCLUDE_DIR})
```

The files of the library are independent from each others, so if you only need one specific feature, you can use only the header that contains it.   
Actually the only file with a dependency is `process.hpp` which defines more features if `pipe.hpp` is available.

## Usage

Here is a short example using Not Enough Standards:

#### main.cpp
```cpp
#include <iostream>
#include <nes/process.hpp>

int main()
{
    //nes::this_process namespace can be used to modify current process or get informations about it.
    std::cout << "Current process has id " << nes::this_process::get_id() << std::endl; 
    std::cout << "Its current directory is \"" << nes::this_process::working_directory() << "\"" << std::endl;

    //Create a child process
    nes::process other{"other_process", {"Hey!", "\\\"12\"\"\\\\", "\\42\\", "It's \"me\"!"}, nes::process_options::grab_stdout};
    
    //Read the entire standard output of the child process. (nes::process_options::grab_stdout must be specified on process creation)
    std::cout << other.stdout_stream().rdbuf() << std::endl;

    //As a std::thread, a nes::process must be joined if it is not detached.
    if(other.joinable())
        other.join();

    //Once joined, we can check its return code.
    std::cout << "Other process ended with code: " << other.return_code() << std::endl;
}
```

#### other.cpp

```cpp
#include <iostream>
#include <nes/process.hpp>

int main(int argc, char** argv)
{
    //Output some informations about this process
    std::cout << "Hello world! I'm Other!\n";
    std::cout << "You gave me " << argc << " arguments:";
    for(int i{}; i < argc; ++i)
        std::cout << "[" << argv[i] << "] ";
    std::cout << '\n';
    std::cout << "My working directory is \"" << nes::this_process::working_directory() << "\"" << std::endl;
}
```

#### Output

```
Current process has id 3612
Its current directory is "/..."
Hello world! I'm Other!
You gave me 5 arguments:[not_enough_standards_other.exe] [Hey!] [\"12""\\\] [\42\] [It's "me"!] 
My working directory is "/..."

Other process ended with code: 0
```

## License

Not Enough Standards use the [MIT license](https://opensource.org/licenses/MIT).
