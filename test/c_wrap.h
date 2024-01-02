#include "c_wrap.h"
#include <cassert>
#include <string>
#include <vector>

int main() {
    // add
    auto interpreter = act::Interpreter();
    int a = 0, b = 1, c = 2;
    a = interpreter.call<int>("test_cpy_api_example", "add", b, c);
    std::cout
        << "cpp: a = interpreter.call<int>(\"test_cpy_api_example\", \"add\", b, c) = "
        << a << '\n';
    assert(a == 3);

    // get_len
    std::string sa = "", sb = "Hello,", sc = "World!";
    sa = interpreter.call<std::string>("test_cpy_api_example", "add", sb, sc);
    std::cout
        << "cpp: sa = interpreter.call<int>(\"test_cpy_api_example\", \"add\", sb, sc) = "
        << sa << '\n';
    assert(sa == "Hello,World!");

    std::vector<int> vec = {1, 2, 3, 42};
    auto len = interpreter.call<int>("test_cpy_api_example", "get_len", vec);
    std::cout
        << "cpp: auto len = interpreter.call(\"test_cpy_api_example\", \"get_len\", vec) = "
        << len << "\n";
    assert(len == 4);

    std::vector<std::string> svec = {"Hello", ", ", "World", "!\n"};
    auto slen = interpreter.call<int>("test_cpy_api_example", "get_len", svec);
    std::cout
        << "cpp: auto slen = interpreter.call<int>(\"test_cpy_api_example\", \"get_len\", svec) = "
        << slen << "\n";
    assert(len == 4);

    // red_print
    const std::string print_str = "This line should be red!";
    interpreter.call("test_cpy_api_example", "red_print", print_str);
    // std::cout << ret << "\n";

    // multi_type_fun
    std::vector<std::variant<
        int, std::string, std::vector<std::variant<int, std::string>>>>
        multi_vec{
            1, 2, "3", std::vector<std::variant<int, std::string>>{1, 2, "3"}};
    interpreter.call("test_cpy_api_example", "multi_type_fun", multi_vec);

    return 0;
}
