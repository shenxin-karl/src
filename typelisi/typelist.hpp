#ifndef _TYPELIST_HPP_
#define _TYPELIST_HPP_
#include <type_traits>

template<typename... Types>
struct TypeList {
};

/* 判断容器是否为空 */
template<typename T>
struct IsEmptyT;

template<typename... Types>
struct IsEmptyT<TypeList<Types...>> {
    constexpr static bool value = (sizeof...(Types) == 0);
};

template<typename T>
static constexpr bool IsEmpty = IsEmptyT<T>::value;


/* 获取容器的元素数量 */
template<typename T>
struct SizeT;

template<typename... Types>
struct SizeT<TypeList<Types...>> {
    static constexpr unsigned int value = sizeof...(Types);
};

template<typename T>
static constexpr unsigned int Size = SizeT<T>::value;


/* 获取容器的第一个元素 */
template<typename T>
struct FrontT;

template<typename Head, typename... Tails>
struct FrontT<TypeList<Head, Tails...>> {
    using Type = Head;
};

template<typename T>

using Front = typename FrontT<T>::Type;


/* 删除容器的第一元素 */
template<typename T>
struct PopFrontT;

template<typename Head, typename... Tails>
struct PopFrontT<TypeList<Head, Tails...>> {
    using Type = TypeList<Tails...>;
};

template<typename T>
using PopFront = typename PopFrontT<T>::Type;

/* 往容器头部添加一个元素 */
template<typename T, typename NewElement>
struct PushFrontT;

template<typename... Types, typename NewElement>
struct PushFrontT<TypeList<Types...>, NewElement> {
    using Type = TypeList<NewElement, Types...>;
};

template<typename T, typename NewElement>
using PushFront = typename PushFrontT<T, NewElement>::Type;


/* 获取容器的最后一个元素 */
template<typename T>
struct BackT;

template<typename... Types>
struct BackT<TypeList<Types...>> {
    using Type = typename BackT<PopFront<TypeList<Types...>>>::Type;
};

template<typename Tail>
struct BackT<TypeList<Tail>> {
    using Type = Tail;
};

template<typename T>
using Back = typename BackT<T>::Type;

/* 往容器末尾添加一个元素 */
template<typename T, typename NewElement>
struct PushBackT;

template<typename... Types, typename NewElement>
struct PushBackT<TypeList<Types...>, NewElement> {
    using Type = TypeList<Types..., NewElement>;
};

template<typename T, typename NewElement>
using PushBack = typename PushBackT<T, NewElement>::Type;


/* 反转容器 */
template<typename T>
struct ReserveT;

template<typename... Types>
struct ReserveT<TypeList<Types...>> {
private:
    using Container = TypeList<Types...>;
    using Tail = PopFront<Container>;
    using Head = Front<Container>;
public:
    using Type = PushBack<typename ReserveT<Tail>::Type, Head>; 
};

template<>
struct ReserveT<TypeList<>> {
    using Type = TypeList<>;
};

template<typename T>
using Reserve = typename ReserveT<T>::Type;


/* 删除容器的最后一个元素 */
template<typename T>
struct PopBackT;

template<typename... Types>
struct PopBackT<TypeList<Types...>> {
private:
    using Container = TypeList<Types...>;
    using Tail = PopFront<Container>;
    using Head = Front<Container>;
public:
    using Type = PushFront<typename PopBackT<Tail>::Type, Head>;
};

template<typename Tail>
struct PopBackT<TypeList<Tail>> {
    using Type = TypeList<>;
};

template<typename T>
using PopBack = typename PopBackT<T>::Type;


/* TransForm */
template<typename T, template<typename> class MetaFun>
struct TransFormT;

template<typename... Types, template<typename> class MetaFun>
struct TransFormT<TypeList<Types...>, MetaFun> {
    using Type = TypeList<MetaFun<Types>...>;
};
    
template<typename T, template<typename> class MetaFun>
using TransForm = typename TransFormT<T, MetaFun>::Type;


/* Accmulate */
template<typename List,
         template<typename X, typename Y> class F,
         typename I,
         bool Empty = IsEmpty<List>>
struct AccmulateT;

template<typename List,
         template<typename X, typename Y> class F,
         typename I>
struct AccmulateT<List, F, I, false> 
    : public AccmulateT<PopFront<List>, F, typename F<I, Front<List>>::Type> {
};

template<typename List, 
         template<typename X, typename Y> class F,
         typename I>
struct AccmulateT<List, F, I, true> {
    using Type = I;             
};

template<typename List, 
         template<typename X, typename Y> class F,
         typename I>
using Accmulate = typename AccmulateT<List, F, I>::Type;


/* 获取对应下标的元素 */
using std::size_t;
template<typename List, size_t N, bool Empty = IsEmpty<List>>
struct IndexOfT;

template<typename List, size_t N>
struct IndexOfT<List, N, false> : public IndexOfT<PopFront<List>, N - 1> {
};

template<typename List>
struct IndexOfT<List, 0, false> {
    using Type = Front<List>;
};

template<typename List, size_t N>
using IndexOf = typename IndexOfT<List, N>::Type;


/* 获取容器中的最大元素 */
template<typename List, bool Empty = IsEmpty<List>>
struct LargestTypeT;

template<typename List>
struct LargestTypeT<List, false> {
private:
    using Contender = Front<List>;
    using Best = typename LargestTypeT<PopFront<List>>::Type;
public:
    using Type = std::conditional_t<(sizeof(Contender) > sizeof(Best)), Contender, Best>;
};

template<typename List>
struct LargestTypeT<List, true> {
    using Type = char;
};

template<typename T>
using LargestType = typename LargestTypeT<T>::Type;


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
         template<typename X, typename Y> class Compare>
struct InsertSortedT<List, Element, Compare, true> 
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
struct SmallerThanT {
    static constexpr bool value = sizeof(X) <= sizeof(Y);
};

#endif
