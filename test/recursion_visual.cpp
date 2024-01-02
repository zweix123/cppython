#include "recurvis.h"
#include <iostream>

int fib(int n) {
    // act::rv().set_file_name("fib.png");
    RV;
    // 这个宏是需要"唯一"额外添加的

    if (n <= 2) {
        rv_out << "n = " << n << "\nret = 1";
        // 以这样的形式控制要显示的内容, 支持换行
        return 1;
    }
    auto ret = fib(n - 1) + fib(n - 2);
    rv_out << "n = " << n << "\nret = " << ret;
    return ret;
}

int main() {
    std::cout << fib(5) << "\n";
    return 0;
}