#ifndef _TYPELIST_HPP_
#define _TYPELIST_HPP_
#include <cstddef>

template<typename... Types>
struct TypeList {
};


template<typename T>
struct IsEmptyT;

template<typename... Types>
struct IsEmptyT<TypeList<Types...>> {
    static constexpr bool value = (sizeof...(Types) == 0);
};

template<typename T>
static constexpr bool IsEmpty = IsEmptyT<T>::value;

template<typename T>
struct FrontT;

template<typename Head, typename... Tail>
struct FrontT<TypeList<Head, Tail...>> {
    using Type = Head;
};

template<typename T>
using Front = typename FrontT<T>::Type;


template<typename T>
struct PopFrontT;

template<typename Head, typename... Tail>
struct PopFrontT<TypeList<Head, Tail...>> {
    using Type = TypeList<Tail...>;
};

template<typename T>
using PopFront = typename PopFrontT<T>::Type;


template<typename T, size_t N, bool Empty = IsEmptyT<T>::value>
struct IndexOfT;

template<typename T, size_t N>
struct IndexOfT<T, N, false> {
    using Type = typename IndexOfT<PopFront<T>, N - 1>::Type;
};

template<typename T>
struct IndexOfT<T, 0, false> {
    using Type = Front<T>;
};

template<typename T, size_t N>
using IndexOf = typename IndexOfT<T, N>::Type;

#endif
