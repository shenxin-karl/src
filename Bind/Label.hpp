#ifndef _LABEL_HPP_
#define _LABEL_HPP_
#include <type_traits>
#include <cstddef>

template<size_t N>
struct Label {
    static constexpr size_t value = N - 1;
};

template<typename T>
struct IsLabelT : public std::false_type {
};

template<size_t N>
struct IsLabelT<Label<N>> : public std::true_type {
};


template<typename T>
static constexpr bool IsLabel = IsLabelT<T>::value;


#define DEF_LABEL(i) static auto _##i = Label<i>{}

DEF_LABEL(1);
DEF_LABEL(2);
DEF_LABEL(3);
DEF_LABEL(4);
DEF_LABEL(5);
DEF_LABEL(6);

#endif // ! _LABEL_HPP_
