#ifndef _EQUALITY_HPP_
#include <type_traits>
#include <exception>

/*
 * 类功能: 自定义异常 
 */
class NotEquality : public std::exception {
};

/*
 * 元函数: 判断 模板参数 类型, 是否用 == 运算符, 并且能够转换成 bool
 */
template<typename T>
class HasEqualityT {
    static void *aux(bool);

    template<typename U>
    static std::true_type test(decltype(aux(std::declval<U>() == std::declval<U>())), 
                        decltype(aux(!(std::declval<U>() == std::declval<U>()))));

    template<typename U>
    static std::false_type test(...);
public:
    static constexpr bool value = decltype(test<T>(nullptr, nullptr))::value;
};


template<typename T, bool HasEquality = HasEqualityT<T>::value>
class Equality;

template<typename T>
class Equality<T, true> {
public:
    static bool equality(T const &first, T const &second) 
    {
        return first == second; 
    }
};

template<typename T>
class Equality<T, false> {
public:
    static bool equality(T const &, T const &)
    {
        throw NotEquality();
    }
};

#endif // _HASEQUALITY_HPP_
