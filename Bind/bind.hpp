#ifndef _BIND_HPP_ 
#define _BIND_HPP_
#include <tuple>
#include "TypeList.hpp"
#include "Label.hpp"
#include <iostream>

using std::tuple;

template<typename Functor, typename... Args> class MyBind;

template<typename Functor, typename... Args>
MyBind<Functor, Args...> Bind(Functor &&functor, Args&&... args);


template<typename TL, size_t N, typename... SArgs, typename... RArgs>
decltype(auto) replaceGet(tuple<SArgs...> &stuple, tuple<RArgs...> &rtuple)
{
    using Type = IndexOf<TL, N>;
    
    if constexpr (IsLabel<Type>)
        return std::get<Type::value>(rtuple);
    else
        return std::get<N>(stuple); 
}

template<typename Functor, typename... Args>
class MyBind {
    using FunctorT = typename std::decay<Functor>::type;
    using SaveArgsT = tuple<typename std::decay<Args>::type ...>;

    FunctorT    *m_functor;                                 /* 调用表达式 */
    SaveArgsT   *m_saveArgs;                                /* 绑定的参数 */ 
    size_t      *m_size;                                    /* 统计数量 */
    using ArgsTypeT = TypeList<std::decay_t<Args>...>;      /* 类型容器 */
private:
    friend MyBind<Functor, Args...> Bind<Functor, Args...>(Functor &&, Args&&...);      /* 声明 友元 */

    MyBind(Functor &&functor, Args&&... args) 
        : m_functor(new FunctorT(std::forward<Functor>(functor))), 
          m_saveArgs(new SaveArgsT(std::forward<Args>(args)...)), m_size(new size_t(1))
    { }

    template<typename... RArgs, size_t... Indices>
    decltype(auto) invoke(tuple<RArgs...> &rtuple, std::integer_sequence<size_t, Indices...>)
    {
        return (*m_functor)(replaceGet<ArgsTypeT, Indices>(*m_saveArgs, rtuple)...);
    }

public:
    template<typename... RArgs>
    decltype(auto) operator()(RArgs&&... args) 
    {
        using Indices = std::make_index_sequence<sizeof...(Args)>;
        tuple<std::decay_t<RArgs>...> replaceArgs(std::forward<RArgs>(args)...);

        return invoke(replaceArgs, Indices{});
    }

    MyBind(MyBind const &other) : m_functor(other.m_functor), m_saveArgs(other.m_saveArgs), m_size(other.m_size)
    {
        ++(*m_size);
    }

    MyBind(MyBind &&other) : m_functor(other.m_functor), m_saveArgs(other.m_saveArgs), m_size(other.m_size)
    {
        other.m_functor = nullptr;
        other.m_saveArgs = nullptr;
        other.m_size = nullptr;
    }

    MyBind &operator=(MyBind const &other) = delete;

    MyBind &operator=(MyBind &&other)
    {
        m_functor = other.m_functor;
        m_saveArgs = other.m_saveArgs;
        m_size = other.m_size;

        other.m_functor = nullptr;
        other.m_saveArgs = nullptr;
        other.m_size = nullptr;
        return *this;
    }

    ~MyBind()
    {
        if (m_size != nullptr) {
            --(*m_size);
            if (*m_size == 0) {
                delete m_size;
                delete m_functor;
                delete m_saveArgs;
            }
            m_saveArgs = nullptr;
            m_size = nullptr;
            m_functor = nullptr;
        }
    }
};


template<typename Functor, typename... Args>
MyBind<Functor, Args...> Bind(Functor &&functor, Args&&... args)
{
    return MyBind<Functor, Args...>(std::forward<Functor>(functor), std::forward<Args>(args)...); 
}

#endif // ! _BIND_HPP_ 
