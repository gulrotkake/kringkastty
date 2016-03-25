#ifndef TSETSE_HPP
#define TSETSE_HPP

#pragma once

#include <memory>
#include <string>
#include <list>
#include <sstream>
#include <iostream>
#include <iterator>

class TestSystem {
    TestSystem() {}
    std::list<std::string> _failures;
    std::list< std::pair<std::string, void(*)()> > tests;
    std::string context;

    TestSystem( const TestSystem& );
    const TestSystem& operator=( const TestSystem& );
public:

    static TestSystem * get_instance() {
        static std::auto_ptr<TestSystem> instance;
        if (instance.get() == 0) {
            instance.reset(new TestSystem);
        }
        return instance.get();
    }

    void check(const char *filename, int line, bool expr, const char *exprs) {
        std::ostringstream oss;
        if (!expr) {
            oss << "\033[1;31m" << filename << ':' << line << " \033[0;31mIn test '" << context
                << "', expression '" << exprs << "' failed. \033[0m" << std::endl;
            _failures.push_back(oss.str());
            oss.str("");
        }
    }

    const std::list<std::string> * failures() const {
        return _failures.size()? &_failures : NULL;
    }

    void add(const char *name, void(*func)() ) {
        tests.push_back(std::make_pair(name, func));
    }

    void run() {
        typedef std::list< std::pair<std::string, void(*)()> > test_list_t;
        test_list_t::const_iterator itr(tests.begin());
        test_list_t::const_iterator end(tests.end());
        for (; itr!=end; ++itr) {
            size_t a = _failures.size();
            context = itr->first;
            std::cout << "Executing test " << context << "... ";
            std::cout.flush();
            (*itr->second)();
            size_t b = _failures.size();
            std::cout << "\033[1;" << (a!=b? "31mfailed" : "32mok")
                      << "\033[0m" << std::endl;
        }
    }
};

struct add_test_to_instance {
    add_test_to_instance(const char *name, void(*func)()) {
        ::TestSystem::get_instance()->add(name, func);
    }
};

#define CHECK(val) \
    TestSystem::get_instance()->check(__FILE__, __LINE__, val, #val);

#define TEST(name)                                                   \
    void test_##name();                                              \
    namespace {                                                      \
    add_test_to_instance add_##name(#name, &::test_##name);          \
    }                                                                \
    void test_##name()

int main() {
    ::TestSystem::get_instance()->run();
    if (const std::list<std::string> *f = TestSystem::get_instance()->failures()) {
        std::cerr << "Failures:" << std::endl;
        std::copy(f->begin(), f->end(), std::ostream_iterator<std::string>(std::cerr));
        return 1;
    }
    return 0;
}

#endif // TSETSE_HPP
