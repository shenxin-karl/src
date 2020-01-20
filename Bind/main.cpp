#include <iostream>
#include "bind.hpp"
#include <functional>

int main(void)
{
    auto func = [](int a, int b, int c, int d) {
        std::cout << "a = " << a << std::endl
                  << "b = " << b << std::endl
                  << "c = " << c << std::endl
                  << "d = " << d << std::endl;
    };

    auto binfunc = Bind(func, _1, 20, _2, 40);
    //binfunc(10, 30);
    
    auto const &bf = std::bind(func, 10, 20, 30, 40);

    std::cout << typeid(bf).name() << std::endl;
}
