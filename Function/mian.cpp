#include <iostream>
#include "Functior.hpp"


void print1()
{
    std::cout << "hello world1\n";
}

int main(void)
{
    auto print2 = []() {
        std::cout << "hello world2\n";
    };

    Function<void()> f1(print1);
    f1();

    Function<void()> f2(print2);
    f2();

    f2 = std::move(f1);
    f2();

    system("pause");
}