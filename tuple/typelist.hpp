#ifndef _TYPELIST_HPP_
#define _TYPELIST_HPP_

namespace TL {
    template<typename... Types>
    struct TypeList {
    };

    template<typename List>
    struct SizeT;

    template<typename... Types>
    struct SizeT<TypeList<Types...>> {
        static constexpr unsigned int value = sizeof...(Types);
    };
}


#endif
