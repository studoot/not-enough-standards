#include <iostream>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <iterator>
#include <iomanip>
#include <array>

#include <nes/pipe.hpp>
#include <nes/shared_library.hpp>
#include <nes/process.hpp>
#include <nes/shared_memory.hpp>
#include <nes/named_mutex.hpp>
#include <nes/semaphore.hpp>
#include <nes/named_semaphore.hpp>
#include <nes/hash.hpp>
#include <nes/thread_pool.hpp>

#if defined(NES_WIN32_SHARED_LIBRARY)
    #define NES_EXAMPLE_EXPORT __declspec(dllexport)
#elif defined(NES_POSIX_SHARED_LIBRARY) && defined(__GNUC__)
    #define NES_EXAMPLE_EXPORT __attribute__((visibility("default")))
#else
    #define NES_EXAMPLE_EXPORT //The example may not work properly in this case.
#endif

#if defined(NES_WIN32_PROCESS)
    constexpr const char* other_path{"not_enough_standards_other.exe"};
#elif defined(NES_POSIX_PROCESS)
    constexpr const char* other_path{"not_enough_standards_other"};
#endif

extern "C" NES_EXAMPLE_EXPORT void foo(int i);
extern "C" NES_EXAMPLE_EXPORT void foo(int i)
{
    std::cout << "Hello " << i << "!" << std::endl;
}

static void shared_library_example()
{
    nes::shared_library lib{nes::load_current};

    auto foo_func = lib.load<void(int)>("foo");
    if(foo_func)
        foo_func(42);
}

enum class data_type : std::uint32_t
{
    uint32 = 1,
    float64,
    string
};

static void a_thread(nes::basic_pipe_istream<char>& is) noexcept
{
    while(is)
    {
        data_type type{};
        is.read(reinterpret_cast<char*>(&type), sizeof(data_type));
        if(type == data_type::uint32)
        {
            std::uint32_t value{};
            is.read(reinterpret_cast<char*>(&value), sizeof(std::uint32_t));

            std::cout << "Received an unsigned integer: " << value << std::endl;
        }
        else if(type == data_type::float64)
        {
            double value{};
            is.read(reinterpret_cast<char*>(&value), sizeof(double));

            std::cout << "Received a double: " << value << std::endl;
        }
        else if(type == data_type::string)
        {
            std::uint64_t size{};
            is.read(reinterpret_cast<char*>(&size), sizeof(std::uint64_t));
            std::string str{};
            str.resize(size);
            is.read(reinterpret_cast<char*>(std::data(str)), static_cast<std::streamsize>(size));

            std::cout << "Received a string: " << str << std::endl;
        }
    }
}

static void pipe_example()
{
    auto&&[is, os] = nes::make_anonymous_pipe();

    std::thread thread{a_thread, std::ref(is)};

    for(std::uint32_t i{1}; i < 20; ++i)
    {
        if((i % 3) == 0)
        {
            const data_type type{data_type::uint32};
            os.write(reinterpret_cast<const char*>(&type), sizeof(data_type));

            os.write(reinterpret_cast<const char*>(&i), sizeof(std::uint32_t));
        }
        else if((i % 3) == 1)
        {
            const data_type type{data_type::float64};
            os.write(reinterpret_cast<const char*>(&type), sizeof(data_type));

            const double value{1.0 / i};
            os.write(reinterpret_cast<const char*>(&value), sizeof(double));
        }
        else if((i % 3) == 2)
        {
            const data_type type{data_type::string};
            os.write(reinterpret_cast<const char*>(&type), sizeof(data_type));

            const std::string str{"Hello " + std::to_string(i) + "!"};
            const std::uint64_t size{std::size(str)};
            os.write(reinterpret_cast<const char*>(&size), sizeof(std::uint64_t));
            os.write(std::data(str), static_cast<std::streamsize>(size));
        }
    }

    os.close();

    if(thread.joinable())
        thread.join();
}

static void another_thread(const std::array<std::uint64_t, 8>& data, nes::semaphore& semaphore)
{
    const auto tp1{std::chrono::high_resolution_clock::now()};
    for(std::size_t i{}; i < 8; ++i)
    {
        semaphore.acquire();

        const auto elapsed_time{std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tp1)};
        std::cout << "Value " << i << " ready after " << elapsed_time.count() << "ms: " << data[i] << std::endl;
    }
}

static void semaphore_example()
{
    std::array<std::uint64_t, 8> data{0, 1};
    nes::semaphore semaphore{2};
    std::thread thread{another_thread, std::cref(data), std::ref(semaphore)};

    for(std::size_t i{2}; i < 8; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{250});
        data[i] = i * i;
        semaphore.release();
    }

    if(thread.joinable())
        thread.join();
}

static void named_pipe_example()
{
    nes::process other{other_path, std::vector<std::string>{"named pipe example"}, nes::process_options::grab_stdout};

    nes::pipe_ostream os{"nes_example_pipe"};
    if(!os)
        throw std::runtime_error{"Failed to open pipe."};

    for(std::uint32_t i{1}; i < 20; ++i)
    {
        if((i % 3) == 0)
        {
            const data_type type{data_type::uint32};
            os.write(reinterpret_cast<const char*>(&type), sizeof(data_type));

            os.write(reinterpret_cast<const char*>(&i), sizeof(std::uint32_t));
        }
        else if((i % 3) == 1)
        {
            const data_type type{data_type::float64};
            os.write(reinterpret_cast<const char*>(&type), sizeof(data_type));

            const double value{1.0 / i};
            os.write(reinterpret_cast<const char*>(&value), sizeof(double));
        }
        else if((i % 3) == 2)
        {
            const data_type type{data_type::string};
            os.write(reinterpret_cast<const char*>(&type), sizeof(data_type));

            const std::string str{"Hello " + std::to_string(i) + "!"};
            const std::uint64_t size{std::size(str)};
            os.write(reinterpret_cast<const char*>(&size), sizeof(std::uint64_t));
            os.write(std::data(str), static_cast<std::streamsize>(size));
        }
    }
    os.close();

    std::cout << other.stdout_stream().rdbuf() << std::endl;

    if(other.joinable())
        other.join();

    std::cout << "Other process ended with code: " << other.return_code() << std::endl;
}

static void process_example()
{
    std::cout << "Current process has id " << nes::this_process::get_id() << " and its current directory is \"" << nes::this_process::working_directory() << "\"" << std::endl;

    nes::process other{other_path, {"Hey!", "\\\"12\"\"\\\\\\", "\\42\\", "It's \"me\"!"}, nes::process_options::grab_stdout};
    std::cout << other.stdout_stream().rdbuf() << std::endl;

    if(other.joinable())
        other.join();

    std::cout << "Other process ended with code: " << other.return_code() << std::endl;
}

static void process_kill_example()
{
    nes::process other{other_path, std::vector<std::string>{"process kill example"}, nes::process_options::grab_stdout};
    std::this_thread::sleep_for(std::chrono::seconds{3});
    other.kill();

    std::cout << other.stdout_stream().rdbuf() << std::endl;
    std::cout << "Shut up." << std::endl;
    std::cout << "Other process ended with code: " << other.return_code() << std::endl;
}

static void shared_memory_example()
{
    nes::shared_memory memory{"nes_example_shared_memory", sizeof(std::uint64_t)};
    auto value{memory.map<std::uint64_t>(0)};
    *value = 42;

    nes::process other{other_path, std::vector<std::string>{"shared memory example"}, nes::process_options::grab_stdout};
    std::cout << other.stdout_stream().rdbuf() << std::endl;

    if(other.joinable())
        other.join();

    std::cout << "Other process ended with code: " << other.return_code() << std::endl;
    std::cout << "The value in shared memory is: " << *value << std::endl;
}

static void named_mutex_example()
{
    nes::named_mutex mutex{"nes_example_named_mutex"};
    std::unique_lock lock{mutex};

    nes::process other{other_path, std::vector<std::string>{"named mutex example"}, nes::process_options::grab_stdout};

    std::this_thread::sleep_for(std::chrono::milliseconds{500});
    lock.unlock();

    std::cout << other.stdout_stream().rdbuf() << std::endl;

    if(other.joinable())
        other.join();
}

static void timed_named_mutex_example()
{
    nes::timed_named_mutex mutex{"nes_example_timed_named_mutex"};
    std::unique_lock lock{mutex};

    nes::process other{other_path, std::vector<std::string>{"timed named mutex example"}, nes::process_options::grab_stdout};

    std::this_thread::sleep_for(std::chrono::milliseconds{1000});
    lock.unlock();

    std::cout << other.stdout_stream().rdbuf() << std::endl;

    if(other.joinable())
        other.join();
}

static void named_semaphore_example()
{
    nes::named_semaphore semaphore{"nes_example_named_semaphore"};

    nes::process other{other_path, std::vector<std::string>{"named semaphore example"}, nes::process_options::grab_stdout};

    for(std::size_t i{}; i < 8; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        semaphore.release();
    }

    std::cout << other.stdout_stream().rdbuf() << std::endl;

    if(other.joinable())
        other.join();
}

static void hash_example()
{
    nes::hash<std::variant<std::string_view, double>> hash{};

    std::cout << nes::from_hash_value<std::uint64_t>(hash("Hello world!")) << std::endl;
}

/*
using hrc = std::chrono::high_resolution_clock;

static void fake_thread_pool_test()
{
    const auto begin = hrc::now();

    volatile std::uint64_t accumulator{};

    for(std::uint64_t i{}; i < 100; ++i)
    {
        for(std::size_t j{}; j < 10000000; ++j)
        {
            accumulator += i;
        }
    }

    std::cout << "Mono thread output: " << accumulator << "; time: " << std::chrono::duration_cast<std::chrono::microseconds>(hrc::now() - begin).count() << "us" << std::endl;
}
*//*
static void thread_pool_test()
{
    nes::thread_pool pool{};
    pool.reserve(10);
    std::vector<std::future<std::uint64_t>> futures{};
    futures.reserve(10);

    for(std::uint64_t i{}; i < 100; ++i)
    {
        futures.emplace_back(pool.push([i]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
            return i;
        }));
    }

    std::uint64_t accumulator{};
    for(auto&& future : futures)
    {
        accumulator += future.get();
    }

    std::cout << "x" << std::flush;
}

static void thread_pool_chain_test()
{
    using clock = std::chrono::high_resolution_clock;

    struct task_data
    {
        clock::time_point begin{};
        clock::time_point end{};
        std::thread::id thread{};
    };

    struct task_context
    {
        std::array<task_data, 5> data{};
        std::future<void> future{};
    };

    nes::thread_pool pool{};

    std::vector<task_context> contexts{};
    contexts.resize(10);

    const auto begin{clock::now()};

    for(std::size_t i{}; i < std::size(contexts); ++i)
    {
        contexts[i].future = pool.chain([&context = contexts[i]]
        {
            context.data[0].begin = clock::now();
            context.data[0].thread = std::this_thread::get_id();
            std::this_thread::sleep_for(std::chrono::milliseconds{500});
            context.data[0].end = clock::now();
        })
        .then([&context = contexts[i]]
        {
            context.data[1].begin = clock::now();
            context.data[1].thread = std::this_thread::get_id();
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
            context.data[1].end = clock::now();
        })
        .with([&context = contexts[i]]
        {
            context.data[2].begin = clock::now();
            context.data[2].thread = std::this_thread::get_id();
            std::this_thread::sleep_for(std::chrono::milliseconds{200});
            context.data[2].end = clock::now();
        })
        .with([&context = contexts[i]]
        {
            context.data[3].begin = clock::now();
            context.data[3].thread = std::this_thread::get_id();
            std::this_thread::sleep_for(std::chrono::milliseconds{300});
            context.data[3].end = clock::now();
        })
        .then([&context = contexts[i]]
        {
            context.data[4].begin = clock::now();
            context.data[4].thread = std::this_thread::get_id();
            std::this_thread::sleep_for(std::chrono::milliseconds{500});
            context.data[4].end = clock::now();
        })
        .push();
    }

    std::cout << std::setprecision(2);

    const auto print_time = [begin](clock::time_point time)
    {
        std::cout << std::fixed << std::chrono::duration<double>(time - begin).count();
    };

    const auto print_task = [print_time](std::size_t i, const task_data& data)
    {
        print_time(data.begin);
        std::cout << " -> T" << i << " (" << data.thread << ") -> ";
        print_time(data.end);
        std::cout << " | ";
    };

    for(std::size_t i{}; i < std::size(contexts); ++i)
    {
        auto& context{contexts[i]};
        context.future.wait();

        std::cout << "Task " << i << " | ";

        for(std::size_t i{}; i < std::size(context.data); ++i)
        {
            print_task(i, context.data[i]);
        }

        std::cout << std::endl;
    }
}
*/
int main()
{
    try
    {
        shared_library_example();
        pipe_example();
        semaphore_example();
        process_example();
        process_kill_example();
        named_pipe_example();
        shared_memory_example();
        named_mutex_example();
        timed_named_mutex_example();
        named_semaphore_example();
        hash_example();

        //fake_thread_pool_test();
        //thread_pool_test();
        //thread_pool_chain_test();
    }
    catch(const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
}
