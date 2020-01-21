#include <iostream>
#include "tuple.hpp"
//#include <tuple>
#if 0
template<typename... Types>
using Tuple = std::tuple<Types...>;
#endif


int main(void)
{
#if 0
    Tuple<char, short, int, float, long> t('c', 10, 10, 10, 10);

    std::cout << std::endl << "=====" << std::endl;

    Tuple<unsigned char, unsigned short, unsigned int, double, unsigned long> t1(t);
#endif

    auto t2(MakeTuple(10, 20,  'c', 5));
}
