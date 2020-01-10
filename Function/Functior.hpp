#ifndef _FUNCTOR_HPP_
#define _FUNCTOR_HPP_
#include <type_traits>
#include <memory>
#include <cassert>
#include "Equality.hpp"

template<typename R, typename... Args> class FunctorBridge;
template<typename Functor, typename R, typename... Args> class SpecificFunctorBridge;
template<typename R, typename... Args> class Function;

/*
 * 抽象基类: 将调用表达式的 返回值 和 参数抽象出来
 */
template<typename R, typename... Args>
class FunctorBridge {
public:
    virtual ~FunctorBridge() {}
    virtual FunctorBridge *clone() const = 0;
    virtual R invoke(Args&&... args) const = 0;
    virtual bool equality(FunctorBridge *) const = 0;
};

template<typename Functor, typename R, typename... Args>
class SpecificFunctorBridge : public FunctorBridge<R, Args...> {
    Functor m_functor;
public:
    template<typename FunctorFwd>
    SpecificFunctorBridge(FunctorFwd &&functor) : m_functor(std::forward<FunctorFwd>(functor)) { }

    virtual SpecificFunctorBridge *clone() const override 
    {
        return new SpecificFunctorBridge(m_functor);
    }

    virtual R invoke(Args&& ...args) const override
    {
        return m_functor(std::forward<Args>(args)...);
    }

    virtual bool equality(FunctorBridge<R, Args...> *other) const override
    {
        if (SpecificFunctorBridge *ospec = dynamic_cast<SpecificFunctorBridge *>(other))
            return Equality<Functor>::equality(m_functor, ospec->m_functor);

        return false;
    }
};


template<typename R, typename... Args>
class Function<R(Args...)> {
    FunctorBridge<R, Args...> *m_bridge = nullptr;

    template<typename OR, typename... OArgs>
    friend class Function;
public:
    Function() 
    { }

    template<typename F>
    Function(F &&functor)
    {
        using Functor = typename std::decay<F>::type;
        m_bridge = new SpecificFunctorBridge<Functor, R, Args...>(std::forward<F>(functor));
    }

    Function(Function const &other) : m_bridge(other.m_bridge->clone()) 
    { }

    Function(Function &&other) : m_bridge(other.m_bridge)
    {
        other.m_bridge = nullptr;
    }

    R operator()(Args&& ...args) const
    {
        assert(!(m_bridge == nullptr));
        return m_bridge->invoke(std::forward<Args>(args)...);
    }

    Function &operator=(Function const &other)
    {
        Function tmp(other);
        swap(*this, tmp);
        return *this;
    }

    Function &operator=(Function &&other)
    {
        if (this == &other)
            return *this;

        this->~Function();
        m_bridge = other.m_bridge;
        other.m_bridge = nullptr;
        return *this;
    }

    ~Function() 
    {
        delete m_bridge;
        m_bridge = nullptr;
    }

    friend void swap(Function &first, Function &second)
    {
        std::swap(first.m_bridge, second.m_bridge);
    }

    friend bool operator==(Function const &first, Function const &second)
    {
        if (first.m_bridge == nullptr || second.birdge == nullptr)
            return first.m_bridge == nullptr && second.birdge == nullptr;

        return first.birdge->equality(second.m_bridge);
    }

    friend bool operator!=(Function const &first, Function const &second)
    {
        return !(first == second); 
    }
};

#endif // !1 