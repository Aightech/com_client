/**
 * @file test_utils.hpp
 * @brief Simple test utilities for com_client unit tests
 *
 * This is a minimal test framework that doesn't require external dependencies.
 * Each test file can be compiled and run independently.
 */

#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include <iostream>
#include <string>
#include <functional>
#include <vector>
#include <chrono>

namespace Test
{

struct TestResult
{
    std::string name;
    bool passed;
    std::string message;
};

class TestRunner
{
public:
    void add_test(const std::string &name, std::function<bool()> test_fn)
    {
        m_tests.push_back({name, test_fn});
    }

    int run()
    {
        int passed = 0;
        int failed = 0;

        std::cout << "\n========================================\n";
        std::cout << "Running " << m_tests.size() << " tests\n";
        std::cout << "========================================\n\n";

        for(const auto &test : m_tests)
        {
            std::cout << "[ RUN  ] " << test.first << std::endl;

            auto start = std::chrono::high_resolution_clock::now();
            bool result = false;
            std::string error_msg;

            try
            {
                result = test.second();
            }
            catch(const std::exception &e)
            {
                error_msg = e.what();
                result = false;
            }
            catch(...)
            {
                error_msg = "Unknown exception";
                result = false;
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            if(result)
            {
                std::cout << "[ PASS ] " << test.first
                          << " (" << duration.count() << "ms)" << std::endl;
                passed++;
            }
            else
            {
                std::cout << "[ FAIL ] " << test.first;
                if(!error_msg.empty())
                    std::cout << " - " << error_msg;
                std::cout << std::endl;
                failed++;
            }
        }

        std::cout << "\n========================================\n";
        std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
        std::cout << "========================================\n\n";

        return failed > 0 ? 1 : 0;
    }

private:
    std::vector<std::pair<std::string, std::function<bool()>>> m_tests;
};

// Assertion macros
#define TEST_ASSERT(condition) \
    do { \
        if(!(condition)) { \
            std::cerr << "  Assertion failed: " #condition \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual) \
    do { \
        if((expected) != (actual)) { \
            std::cerr << "  Assertion failed: expected " << (expected) \
                      << ", got " << (actual) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_NE(not_expected, actual) \
    do { \
        if((not_expected) == (actual)) { \
            std::cerr << "  Assertion failed: expected != " << (not_expected) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

} // namespace Test

#endif // TEST_UTILS_HPP
