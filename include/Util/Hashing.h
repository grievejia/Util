#pragma once

#include <functional>
#include <type_traits>

namespace util {

template <typename T>
inline void hash_combine(std::size_t& seed, const T& v) {
    seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename T, typename F>
size_t hashRange(T first, T last, F&& func) {
    std::size_t seed = 0;
    for (auto itr = first; itr != last; ++itr)
        hash_combine(seed, func(*itr));
    return seed;
}

template <typename T>
size_t hashContainer(const T& container) {
    std::size_t seed = 0;
    for (auto elem : container)
        hash_combine(seed, elem);
    return seed;
}

// This is a generalize hasher for all STL containers (and all custom containers
// that exports value_type typedef). For some reasons, C++11 doesn't specialize
// std::hash for key types that are containers themselves. This hasher tries to
// mimic what boost::hash_combine and boost::hash_range does.
template <typename ContainerType>
class ContainerHasher
{
public:
    using value_type = typename ContainerType::value_type;

    std::size_t operator()(const ContainerType& c) const {
        return hashContainer(c);
    }
};

template <typename T1, typename T2>
std::size_t hashPair(const T1& t1, const T2& t2) {
    std::size_t seed = 0;
    hash_combine(seed, t1);
    hash_combine(seed, t2);
    return seed;
}

template <typename Pair>
struct PairHasher
{
    std::size_t operator()(const Pair& p) const {
        return hashPair(p.first, p.second);
    }
};

template <typename T1, typename T2, typename T3>
std::size_t hashTriple(const T1& t1, const T2& t2, const T3& t3) {
    std::size_t seed = 0;
    hash_combine(seed, t1);
    hash_combine(seed, t2);
    hash_combine(seed, t3);
    return seed;
}

template <typename T1, typename T2, typename T3, typename T4>
std::size_t hashQuadraple(const T1& t1, const T2& t2, const T3& t3,
                          const T4& t4) {
    std::size_t seed = 0;
    hash_combine(seed, t1);
    hash_combine(seed, t2);
    hash_combine(seed, t3);
    hash_combine(seed, t4);
    return seed;
}

template <typename EnumClassType>
size_t hashEnumClass(EnumClassType e) {
    using RealType = std::underlying_type_t<EnumClassType>;
    return std::hash<RealType>()(static_cast<RealType>(e));
}

template <typename EnumClassType>
struct EnumClassHasher
{
    std::size_t operator()(const EnumClassType& e) const {
        return hashEnumClass(e);
    }
};
}
