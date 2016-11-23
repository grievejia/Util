#pragma once

#include "Util/Variant.h"

namespace util {

namespace detail {

// A utility class allowing constructing variant visitors using lambda
// It is basically a stripped-down version of std::overload.

template <typename... Lambdas>
struct lambda_visitor;

template <typename Lambda1>
struct lambda_visitor<Lambda1> : Lambda1 {
    using lambda_type = Lambda1;
    using lambda_type::operator();

    lambda_visitor(Lambda1&& f) noexcept : Lambda1(std::forward<Lambda1>(f)) {}
};

template <typename Lambda1, typename... Lambdas>
struct lambda_visitor<Lambda1, Lambdas...>
    : Lambda1, lambda_visitor<Lambdas...>::lambda_type {
    using base_type = typename lambda_visitor<Lambdas...>::lambda_type;
    using lambda_type = lambda_visitor;

    using Lambda1::operator();
    using base_type::operator();

    lambda_visitor(Lambda1&& l1, Lambdas&&... lambdas) noexcept
        : Lambda1(std::forward<Lambda1>(l1)),
          base_type(std::forward<Lambdas>(lambdas)...) {}
};
}

template <typename... Lambdas>
auto make_lambda_visitor(Lambdas&&... lambdas) noexcept {
    return detail::lambda_visitor<Lambdas...>(
        std::forward<Lambdas>(lambdas)...);
}
}