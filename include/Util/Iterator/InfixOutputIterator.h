#pragma once

#include <iterator>
#include <ostream>

namespace util {

template <typename T, typename CharT = char,
          typename Traits = std::char_traits<CharT>>
class InfixOstreamIterator
    : public std::iterator<std::output_iterator_tag, void, void, void, void>
{
private:
    std::basic_ostream<CharT, Traits>& os;
    const CharT* delimiter;
    bool isFirstElem;

public:
    using char_type = CharT;
    using traits_type = Traits;
    using ostream_type = std::basic_ostream<CharT, Traits>;

    InfixOstreamIterator(ostream_type& s)
        : os(s), delimiter(0), isFirstElem(true) {}
    InfixOstreamIterator(ostream_type& s, const CharT* d)
        : os(s), delimiter(d), isFirstElem(true) {}

    InfixOstreamIterator& operator=(const T& item) {
        if (delimiter != 0 && !isFirstElem)
            os << delimiter;
        os << item;
        isFirstElem = false;
        return *this;
    }

    InfixOstreamIterator& operator*() { return *this; }
    InfixOstreamIterator& operator++() { return *this; }
    InfixOstreamIterator& operator++(int) { return *this; }
};

template <typename T, typename CharT = char,
          typename Traits = std::char_traits<CharT>>
InfixOstreamIterator<T, CharT, Traits> infix_ostream_iterator(
    std::basic_ostream<CharT, Traits>& s, const CharT* d) {
    return InfixOstreamIterator<T, CharT, Traits>(s, d);
}
}