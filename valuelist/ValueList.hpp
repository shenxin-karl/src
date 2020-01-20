#ifndef _VALUELIST_HPP_
#define _VALUELIST_HPP_
#include <type_traits>
using std::size_t;

template<typename T, T V>
struct ValueT {
    static constexpr T value = V;
};

template<typename T, T... Values>
struct ValueList {
};

/* 判断容器是否为空 */
template<typename List>
struct IsEmptyT;

template<typename T, T... Values>
struct IsEmptyT<ValueList<T, Values...>> {
    static constexpr bool value = sizeof...(Values) == 0;
};

template<typename T>
static constexpr bool IsEmpty = IsEmptyT<T>::value;


/* 获取容器数量 */
template<typename List>
struct SizeT;

template<typename T, T... Values>
struct SizeT<ValueList<T, Values...>> {
    static constexpr size_t value = sizeof...(Values);
};

template<typename List>
static constexpr size_t Size = SizeT<List>::value;

/* 获取容器第一元素 */
template<typename T>
struct FrontT;

template<typename T, T Head, T... Tails>
struct FrontT<ValueList<T, Head, Tails...>> {
    using Type = ValueT<T, Head>;
    static constexpr T value = Head;
};

template<typename T>
using Front = typename FrontT<T>::Type;


/* 删除容器的第一元素 */
template<typename T>
struct PopFrontT;

template<typename T, T Head, T... Tails>
struct PopFrontT<ValueList<T, Head, Tails...>> {
    using Type = ValueList<T, Tails...>;
};

template<typename T>
using PopFront = typename PopFrontT<T>::Type;


/* 添加一个元素到容器中 */
template<typename List, typename NewElement>
struct PushFrontT;

template<typename T, T... Values, T NewElement>
struct PushFrontT<ValueList<T, Values...>, ValueT<T, NewElement>> {
    using Type = ValueList<T, NewElement, Values...>;
};

template<typename T, typename NewElement>
using PushFront = typename PushFrontT<T, NewElement>::Type;

/* 添加一个元素到末尾 */
template<typename List, typename NewElement>
struct PushBackT;

template<typename T, T... Values, T Tail>
struct PushBackT<ValueList<T, Values...>, ValueT<T, Tail>> {
    using Type = ValueList<T, Values..., Tail>;
};

template<typename List, typename NewElement>
using PushBack = typename PushBackT<List, NewElement>::Type;


/* 获取容器的最后一个元素 */
template<typename List>
struct BackT;

template<typename T, T... Values>
struct BackT<ValueList<T, Values...>> 
    : public BackT<PopFront<ValueList<T, Values...>>> {
};

template<typename T, T Tail>
struct BackT<ValueList<T, Tail>> {
    using Type = ValueT<T, Tail>;
};

template<typename List>
using Back = typename BackT<List>::Type;


/* 删除容器最后一个元素 */
template<typename List>
struct PopBackT;

template<typename T, T... Values>
struct PopBackT<ValueList<T, Values...>> {
private:
    using Container = ValueList<T, Values...>;
    using Head = FrontT<Container>;
public:
    using Type = PushFront<PopFront<Container>, Head>;
};

template<typename T, T Tail>
struct PopBackT<ValueList<T, Tail>> {
    using Type = ValueList<T>;
};

template<typename List>
using PopBack = typename PopBackT<List>::Type;

/* transform */
template<typename List, 
         template<typename X> class MetaFun>
struct TransformT;

template<typename T, T... Values,
         template<typename X> class MetaFun>
struct TransformT<ValueList<T, Values...>, MetaFun> {
private:
    using Container = ValueList<T, Values...>;
    using MT = decltype(MetaFun<Front<Container>>::value);
public:
    using Type = ValueList<MT, MetaFun<ValueT<T, Values>>::value...>;
};

template<typename List, template<typename X> class MetaFun>
using Transform = typename TransformT<List, MetaFun>::Type;


/* Accmulate */
template<typename List, 
         template<typename L, typename T> class MetaFun,
         typename I,
         bool Empty = IsEmpty<List>>
struct AccmulateT;

template<typename List,
         template<typename L, typename T> class MetaFun,
         typename I>
struct AccmulateT<List, MetaFun, I, false> 
    : public AccmulateT<PopFront<List>, MetaFun, typename MetaFun<I, Front<List>>::Type> {
};

template<typename List,
         template<typename L, typename T> class MetaFun,
         typename I>
struct AccmulateT<List, MetaFun, I, true> {
    using Type = I;
};

template<typename List,
         template<typename L, typename I> class MetaFun,
         typename I>
using Accmulate = typename AccmulateT<List, MetaFun, I>::Type;


/* 插入排序 */

template<typename T>
struct IdentityT {
    using Type = T;
};

template<typename List,
         typename Element,
         template<typename X, typename Y> class Compare,
         bool Empty = IsEmpty<List>>
struct InsertSortedT;

template<typename List,
         typename Element,
         template<typename X, typename Y> class Compare>
struct InsertSortedT<List, Element, Compare, false> {
private:
    using NewTail = typename std::conditional_t< Compare<Element, Front<List>>::value,
                                                 IdentityT<List>,
                                                 InsertSortedT<PopFront<List>, Element, Compare>>::Type;
    
    using NewHead = std::conditional_t< Compare<Element, Front<List>>::value,
                                        Element,
                                        Front<List>>;
public:
    using Type = PushFront<NewTail, NewHead>;
};

template<typename List,
         typename Element,
         template<typename X, typename Y> class Comapre>
struct InsertSortedT<List, Element, Comapre, true>
    : public PushFrontT<List, Element> {
};
         

template<typename List, 
         template<typename X, typename Y> class Compare,
         bool Empty = IsEmpty<List>>
struct InsertSortT;

template<typename List,
         template<typename X, typename Y> class Compare>
using InsertSort = typename InsertSortT<List, Compare>::Type;        

template<typename List, 
         template<typename X, typename Y> class Compare>
struct InsertSortT<List, Compare, false>
    : public InsertSortedT< InsertSort<PopFront<List>, Compare>,
                            Front<List>,
                            Compare> {
};

template<typename List,
         template<typename X, typename Y> class Compare>
struct InsertSortT<List, Compare, true> {
    using Type = List;
};

template<typename X, typename Y>
struct SmallerThanT;

template<typename T, T X, T Y>
struct SmallerThanT<ValueT<T, X>, ValueT<T, Y>> {
    static constexpr bool value = X < Y;
};

/* 生成序列 */
template<size_t N, typename I = ValueList<size_t>>
struct IndexSequenceT
    : public IndexSequenceT<N - 1, PushFront<I, ValueT<size_t, N - 1>>> {
};

template<typename I>
struct IndexSequenceT<0, I> {
    using Type = I;
};

template<size_t N>
using IndexSequence = typename IndexSequenceT<N>::Type;


/* 反转容器 */
template<typename List, bool Empty = IsEmpty<List>>
struct ReverseT;

template<typename T, T... Valuse>
struct ReverseT<ValueList<T, Valuse...>, false> {
private:
    using Container = ValueList<T, Valuse...>;
    using Head = Front<Container>;
public:
    using Type = PushBack<typename ReverseT<PopFront<Container>>::Type, Head>;
};

template<typename T, T... Values>
struct ReverseT<ValueList<T, Values...>, true> {
    using Type = ValueList<T, Values...>;
};

template<typename List>
using Reverse = typename ReverseT<List>::Type;
#endif
