// Copyright 2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
// Original Author: Ryan Pavlik

#pragma once
#include <iterator>

template <typename Iterator>
using iter_value_t = typename std::iterator_traits<Iterator>::value_type;

/**
 * @brief Sums pairwise, in place, returning the new "past the end" iterator.
 * 
 * In theory can handle non-even number of elements, but I'd rather not.
 * 
 * 
 * @tparam Iterator 
 * @param begin First element to sum
 * @param end Past the last element to sum
 * @return Iterator The new "past the end" iterator
 */
template <typename Iterator>
static inline Iterator sumPairsInPlace(Iterator begin, Iterator end)
{
    Iterator output = begin;
    auto takeInputElt = [&]() -> iter_value_t<Iterator> & {
        auto &ret = *begin;
        begin++;
        return ret;
    };
    while (begin != end)
    {
        auto &a = takeInputElt();
        if (begin == end)
        {
            // odd number of pairs, prefer not to hit this code ever.
            *output = a;
            output++;
            return output;
        }
        auto &b = takeInputElt();
        *output = a + b;
        output++;
    }
    return output;
}

template <typename Iterator>
static inline iter_value_t<Iterator> pairwiseReduce(Iterator begin, Iterator end)
{
    while (std::distance(begin, end) > 1)
    {
        auto newEnd = sumPairsInPlace(begin, end);
        end = newEnd;
    }
    return *begin;
}