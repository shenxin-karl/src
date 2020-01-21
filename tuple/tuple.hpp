#ifndef _TUPLE_HPP_
#define _TUPLE_HPP_
#include <type_traits>
#include <utility>
#include "typelist.hpp"


/* 前向声明 */
template<typename...> class Tuple;
template<unsigned int N> struct TupleGet;
template<unsigned int N, typename... Types> auto &Get(Tuple<Types...> &tuple);
template<unsigned int N, typename... Types> auto const &Get(Tuple<Types...> const &tuple);

/* 空类 Tuple */
template<>
class Tuple<> {
};


template<typename Head, typename... Tails>
class Tuple<Head, Tails...> {
    using TailT = Tuple<Tails...>;
    using TypeListT = TL::TypeList<Head, Tails...>;             /* 计算 Tuple 的size */

    template<typename...>
    friend class Tuple;

    template<unsigned int N, typename... Types>
    friend auto &Get(Tuple<Types...> &);

    template<unsigned int N, typename... Types>
    friend auto const &Get(Tuple<Types...> const &);
private:
    Head        m_head;
    TailT       m_tail;
private:
    Head &getHead() { return m_head; }

    Head const &getHead() const { return m_head; }

    TailT &getTail() { return m_tail; }

    TailT const &getTail() const { return m_tail; }

    Tuple(Head const &head, TailT const &tail) : m_head(head), m_tail(tail) { }
public:
    Tuple() {  }

    template<typename VHead, typename... VTails,
             typename = std::enable_if_t<(sizeof...(Tails) == sizeof...(VTails))>>
    Tuple(VHead &&head, VTails&&... vtails) : m_head(std::forward<VHead>(head)), m_tail(std::forward<VTails>(vtails)...) { }

    template<typename VHead, typename... VTails,
             typename = std::enable_if_t<(sizeof...(Tails) == sizeof...(VTails))>>
    Tuple(Tuple<VHead, VTails...> const &other) : m_head(other.getHead()), m_tail(other.getTail()) { }

    Tuple &operator=(Tuple const &other) 
    {
        this->getHead() = other.getHead();
        this->getTail() = other.getTail();

        return *this;
    }

    constexpr unsigned int size() const
    {
       return TL::SizeT<TypeListT>::value; 
    }

    constexpr bool empty() const
    {
        return size() == 0;
    }

    friend bool operator==(Tuple const &first, Tuple const &second)
    {
        return first.getHead() == second.getHead() &&
               first.getTail() == second.getTail();
    }

    friend bool operator!=(Tuple const &first, Tuple const &second)
    {
        return !(first == second);
    }

    friend bool operator<(Tuple const &first, Tuple const &second)
    {
        return first.getHead() < second.getHead() &&
               first.getTail() < second.getTail();
    }

    friend bool operator>(Tuple const &first, Tuple const &second)
    {
        return first.getHead() > second.getHead() &&
               first.getTail() > second.getTail();
    }

    friend bool operator<=(Tuple const &first, Tuple const &second)
    {
        return (first < second) || (first == second);
    }

    friend bool operator>=(Tuple const &first, Tuple const &second)
    {
        return (first > second) || (first == second);
    }

    friend void swap(Tuple &first, Tuple &second)
    {
        using std::swap;
        swap(first.getHead(), second.getHead());
        swap(first.getTail(), second.getTail());
    }
};


template<unsigned int N>
struct TupleGet {
    template<typename... Types>
    static auto apply(Tuple<Types...> const &tuple)
    {
        return TupleGet<N - 1>::apply(tuple.getTail());
    }

    template<typename... Types>
    static auto apply(Tuple<Types...> &tuple)
    {
        return TupleGet<N - 1>::apply(tuple.getTail());
    }
};

template<>
struct TupleGet<0> {
    template<typename... Types>
    static auto apply(Tuple<Types...> const &tuple)
    {
        return tuple.getHead();
    }

    template<typename... Types>
    static auto apply(Tuple<Types...> &tuple)
    {
        return tuple.getHead();
    }
};

template<unsigned int N, typename... Types>
auto &Get(Tuple<Types...> &tuple)
{
    using TupleT = Tuple<Types...>;
    static_assert((TL::SizeT<typename TupleT::TypeListT>::value <= N), "Get out_of_range\n");

    return TupleGet<N>::apply(tuple);
}

template<unsigned int N, typename... Types>
auto const &Get(Tuple<Types...> const &tuple)
{
    using TupleT = Tuple<Types...>;
    static_assert((TL::SizeT<typename TupleT::TypeListT>::value <= N), "Get out_of_range\n");

    return TupleGet<N>::apply(tuple);
}


template<typename... Types>
auto MakeTuple(Types&&... args)
{
    return Tuple<typename std::decay<Types>::type...>(std::forward<Types>(args)...);
}

#endif
