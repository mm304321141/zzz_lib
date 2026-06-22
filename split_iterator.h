
#pragma once

#ifndef ZZZ_LIB_NODISCARD
#if __cplusplus >= 201703L
#define ZZZ_LIB_NODISCARD [[nodiscard]]
#else
#define ZZZ_LIB_NODISCARD
#endif
#endif

#include <cstdint>
#include <algorithm>
#include <utility>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <string>
#include <cmath>
#include <iterator>
#include <memory>
#include <system_error>
#if __cplusplus >= 201703L && defined(__cpp_lib_charconv)
#include <charconv>
#endif

// Parses an optional leading '-' followed by a run of decimal digits and
// returns its value.  Parsing stops at the first character that is not part of
// the number, so a trailing remainder is allowed (matching std::from_chars).
// On overflow or when no digit can be parsed the result is value-initialised
// (0), mirroring how std::from_chars leaves its output untouched on failure.
// When `status` is non-null it receives std::errc() on success,
// std::errc::result_out_of_range on overflow and std::errc::invalid_argument
// when no digit was consumed.
//
// For unsigned target types the leading '-' is *accepted* and triggers
// modular wrap-around (the same semantics std::stoul exposes): the absolute
// value is parsed as unsigned and then negated in modular arithmetic, so
// "-5" yields UINT_MAX - 4 instead of an invalid_argument error.  The
// std::from_chars implementation does not provide this behaviour for unsigned
// types out of the box, so the C++17 path emulates it explicitly to keep
// observable behaviour identical between the two code paths.
//
// The two code paths below intentionally share the exact same observable
// contract (return value plus reported status):
//   * C++17 and newer use std::from_chars (only for char; other character
//     types fall through to the hand-written path even under C++17).
//   * C++11/14 use a hand-written parser with explicit overflow detection.
template<class char_t, class integer_t> integer_t string_to_integer(char_t const *s, size_t l, std::errc *status = nullptr)
{
#if __cplusplus >= 201703L && defined(__cpp_lib_charconv)
    if constexpr(std::is_same<char_t, char>::value)
    {
        if constexpr(std::is_unsigned<integer_t>::value)
        {
            // std::from_chars rejects a leading '-' for unsigned targets, so
            // emulate std::stoul by parsing the absolute value as unsigned and
            // wrapping around (negation in modular arithmetic).
            if(l != 0 && s[0] == '-')
            {
                integer_t fc_result{};
                auto fc = std::from_chars(s + 1, s + l, fc_result);
                if(status != nullptr)
                {
                    *status = fc.ec;
                }
                if(fc.ec != std::errc())
                {
                    return integer_t();
                }
                return integer_t(integer_t(0) - fc_result);
            }
        }
        integer_t fc_result{};
        auto fc = std::from_chars(s, s + l, fc_result);
        if(status != nullptr)
        {
            *status = fc.ec;
        }
        return fc_result;
    }
#endif
    // C++11/14 hand-written fallback.
    // For both signed and unsigned targets we accept a leading '-': signed
    // types negate the parsed magnitude as usual, while unsigned types perform
    // modular wrap-around (matching std::stoul).
    size_t neg = (l != 0 && s[0] == char_t('-')) ? 1 : 0;
    size_t nc = neg;
    while(nc != l && s[nc] >= char_t('0') && s[nc] <= char_t('9'))
    {
        ++nc;
    }
    if(nc == neg)
    {
        // No digit consumed: align with std::from_chars invalid_argument.
        if(status != nullptr)
        {
            *status = std::errc::invalid_argument;
        }
        return integer_t();
    }
    typedef typename std::make_unsigned<integer_t>::type uinteger_t;
    uinteger_t umax = uinteger_t(-1);
    // Largest representable magnitude: the unsigned maximum for unsigned
    // targets (wrap-around accepts the full range regardless of sign),
    // INT_MAX for non-negative signed values and INT_MAX + 1 for negatives.
    uinteger_t limit = std::is_unsigned<integer_t>::value ? umax : uinteger_t(uinteger_t(umax >> 1) + (neg ? uinteger_t(1) : uinteger_t(0)));
    uinteger_t acc = 0;
    for(size_t i = neg; i != nc; ++i)
    {
        uinteger_t digit = uinteger_t(s[i] - char_t('0'));
        if(acc > (limit - digit) / 10)
        {
            // Overflow: align with std::from_chars (output left untouched).
            if(status != nullptr)
            {
                *status = std::errc::result_out_of_range;
            }
            return integer_t();
        }
        acc = acc * 10 + digit;
    }
    if(status != nullptr)
    {
        *status = std::errc();
    }
    return neg ? integer_t(uinteger_t(uinteger_t(0) - acc)) : integer_t(acc);
}

// Parses an optional leading '-' followed by a decimal floating point number
// ("ddd", "ddd.fff", ".fff", each with an optional 'e'/'E' exponent) and
// returns its value.  When `status` is non-null it receives std::errc() on
// success, std::errc::result_out_of_range on overflow/underflow and
// std::errc::invalid_argument when no digit was consumed, and the parsed value
// of the leading number is returned even if a trailing remainder follows
// (matching std::from_chars).  When `status` is null the legacy contract is
// kept: the value is returned only when the whole input is a valid number,
// otherwise a value-initialised result (0) is returned.
//
// C++17 and newer use std::from_chars (only for char).  The C++11/14 fallback
// accumulates the significant decimal digits into a 64-bit integer mantissa
// together with a base-10 exponent and then scales the mantissa by a power of
// ten.  For the common range (mantissa exactly representable, i.e. <= 2^53, and
// |exponent| <= 22) the single multiply / divide by an exact power of ten is
// correctly rounded and therefore matches std::from_chars bit-for-bit.  Outside
// that range (more than ~15 significant digits or a large exponent) the result
// may differ from std::from_chars by a small number of ULPs because a bit-exact
// fallback would require big-integer arithmetic, which the no-heap / hand-written
// constraint forbids; such inputs exceed the ~17 significant digits a double can
// represent anyway.
template<class char_t, class real_t> real_t string_to_real(char_t const *s, size_t l, std::errc *status = nullptr)
{
#if __cplusplus >= 201703L && defined(__cpp_lib_from_chars)
    if constexpr(std::is_same<char_t, char>::value)
    {
        real_t fc_result{};
        auto fc = std::from_chars(s, s + l, fc_result);
        if(status != nullptr)
        {
            *status = fc.ec;
            return fc_result;
        }
        if(fc.ec == std::errc() && fc.ptr == s + l)
        {
            return fc_result;
        }
        return real_t();
    }
#endif
    // C++11/14 hand-written fallback.
    // Exact powers of ten that are representable in a double without rounding.
    static double const pow10_tab[] = { 1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22 };
    size_t i = 0;
    bool neg = false;
    if(i != l && s[i] == char_t('-'))
    {
        neg = true;
        ++i;
    }
    uint64_t mantissa = 0;     // significant digits, at most 19 of them
    int exp10 = 0;             // base-10 exponent to apply to the mantissa
    int digits = 0;            // number of significant digits in `mantissa`
    int const max_digits = 19; // a uint64_t holds up to 19 decimal digits
    bool any_digit = false;
    bool seen_dot = false;
    for(; i != l; ++i)
    {
        char_t c = s[i];
        if(c == char_t('.'))
        {
            if(seen_dot)
            {
                break;
            }
            seen_dot = true;
            continue;
        }
        if(!(c >= char_t('0') && c <= char_t('9')))
        {
            break;
        }
        any_digit = true;
        int d = int(c - char_t('0'));
        if(mantissa == 0 && d == 0)
        {
            // Leading zero: it does not occupy a significant digit slot, but a
            // leading zero in the fraction still shifts the decimal exponent.
            if(seen_dot)
            {
                --exp10;
            }
        }
        else if(digits < max_digits)
        {
            mantissa = mantissa * 10 + uint64_t(d);
            ++digits;
            if(seen_dot)
            {
                --exp10;
            }
        }
        else
        {
            // Mantissa is full: integer digits keep scaling the magnitude,
            // fraction digits are below the precision of the result and dropped.
            if(!seen_dot)
            {
                ++exp10;
            }
        }
    }
    if(i != l && (s[i] == char_t('e') || s[i] == char_t('E')))
    {
        size_t j = i + 1;
        bool eneg = false;
        if(j != l && (s[j] == char_t('+') || s[j] == char_t('-')))
        {
            eneg = s[j] == char_t('-');
            ++j;
        }
        if(j != l && s[j] >= char_t('0') && s[j] <= char_t('9'))
        {
            int eacc = 0;
            for(; j != l && s[j] >= char_t('0') && s[j] <= char_t('9'); ++j)
            {
                if(eacc < 1000000)
                {
                    eacc = eacc * 10 + int(s[j] - char_t('0'));
                }
            }
            exp10 += eneg ? -eacc : eacc;
            i = j;
        }
        // An 'e' that is not followed by a valid exponent is left unconsumed,
        // matching std::from_chars (the number ends before the 'e').
    }
    std::errc ec = std::errc();
    double value;
    if(!any_digit)
    {
        ec = std::errc::invalid_argument;
        value = 0.0;
    }
    else if(mantissa == 0)
    {
        value = 0.0;
    }
    else
    {
        double m = double(mantissa);
        if(exp10 >= 0)
        {
            if(mantissa <= (uint64_t(1) << 53) && exp10 <= 22)
            {
                value = m * pow10_tab[exp10];
            }
            else
            {
                value = m * std::pow(10.0, double(exp10));
            }
        }
        else
        {
            int e = -exp10;
            if(mantissa <= (uint64_t(1) << 53) && e <= 22)
            {
                value = m / pow10_tab[e];
            }
            else
            {
                value = m / std::pow(10.0, double(e));
            }
        }
        if(std::isinf(value))
        {
            ec = std::errc::result_out_of_range;
        }
        else if(value == 0.0)
        {
            // Non-zero input that underflowed to zero.
            ec = std::errc::result_out_of_range;
        }
    }
    value = neg ? -value : value;
    if(status != nullptr)
    {
        *status = ec;
        return real_t(value);
    }
    if(ec == std::errc() && i == l)
    {
        return real_t(value);
    }
    return real_t();
}

#if __cplusplus >= 201703L
#include <string_view>
template<class char_t = char, class traits_t = std::char_traits<char_t>>
using string_ref = std::basic_string_view<char_t, traits_t>;
#else
template<class char_t = char, class traits_t = std::char_traits<char_t>> class string_ref
{
public:
    typedef traits_t traits_type;
    typedef char_t value_type;
    typedef char_t *pointer;
    typedef char_t const *const_pointer;
    typedef char_t &reference;
    typedef char_t const &const_reference;
    typedef char_t const *iterator, *const_iterator;
    typedef std::reverse_iterator<char_t const *> reverse_iterator, const_reverse_iterator;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

public:
    constexpr string_ref() noexcept : _ptr(), _len()
    {
    }
    constexpr string_ref(string_ref const &) noexcept = default;
    template<class allocator_t> string_ref(std::basic_string<char_t, traits_t, allocator_t> const &str) : _ptr(str.data()), _len(str.length())
    {
    }
    constexpr string_ref(char_t const *s, size_type count) noexcept : _ptr(s), _len(count)
    {
    }
    constexpr string_ref(char_t const *s) noexcept : _ptr(s), _len(traits_type::length(s))
    {
    }

    string_ref &operator=(string_ref const &) noexcept = default;

    constexpr const_iterator begin() const noexcept
    {
        return _ptr;
    }
    constexpr const_iterator end() const noexcept
    {
        return _ptr + _len;
    }
    constexpr const_iterator cbegin() const noexcept
    {
        return _ptr;
    }
    constexpr const_iterator cend() const noexcept
    {
        return _ptr + _len;
    }
    constexpr const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(_ptr);
    }
    constexpr const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(_ptr + _len);
    }
    constexpr const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(_ptr);
    }
    constexpr const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(_ptr + _len);
    }

    constexpr const_reference operator[](size_type index) const noexcept
    {
        return _ptr[index];
    }
    constexpr const_reference at(size_type index) const
    {
        return index >= size() ? throw std::out_of_range("string_ref out of range") : _ptr[index];
    }

    constexpr const_reference front() const noexcept
    {
        return *_ptr;
    }
    constexpr const_reference back() const noexcept
    {
        return *(_ptr + _len - 1);
    }

    constexpr const_pointer data() const noexcept
    {
        return _ptr;
    }

    constexpr size_type size() const noexcept
    {
        return _len;
    }
    constexpr size_type length() const noexcept
    {
        return _len;
    }
    constexpr size_type max_size() const noexcept
    {
        return size_type(-1) / sizeof(value_type);
    }

    constexpr bool empty() const noexcept
    {
        return _len == 0;
    }

    void remove_prefix(size_type n) noexcept
    {
        n = std::min(n, size());
        _ptr += n;
        _len -= n;
    }
    void remove_suffix(size_type n) noexcept
    {
        n = std::min(n, size());
        _len -= n;
    }
    void swap(string_ref &other) noexcept
    {
        std::swap(*this, other);
    }

    template<class allocator_t> operator std::basic_string<char_t, traits_t, allocator_t>() const
    {
        return std::basic_string<char_t, traits_t, allocator_t>(data(), length());
    }

    size_type copy(value_type *dest, size_type count, size_type pos = 0) const noexcept
    {
        if(pos >= size())
        {
            return 0;
        }
        return std::copy_n(_ptr + pos, std::min(count, _len - pos), dest) - dest;
    }
    constexpr string_ref substr(size_type pos = 0, size_type count = npos) const
    {
        return pos > size() ? throw std::out_of_range("string_ref out of range") : string_ref(_ptr + pos, std::min(count, size() - pos));
    }

    constexpr int compare(string_ref v) const noexcept
    {
        return compare_helper(traits_t::compare(data(), v.data(), std::min(size(), v.size())), size(), v.size());
    }
    constexpr int compare(size_type pos1, size_type count1, string_ref v) const noexcept
    {
        return substr(pos1, count1).compare(v);
    }
    constexpr int compare(size_type pos1, size_type count1, string_ref const &v, size_type pos2, size_type count2) const noexcept
    {
        return substr(pos1, count1).compare(v.substr(pos2, count2));
    }
    constexpr int compare(value_type const *s) const noexcept
    {
        return compare(string_ref(s));
    }
    constexpr int compare(size_type pos1, size_type count1, value_type const *s) const noexcept
    {
        return substr(pos1, count1).compare(string_ref(s));
    }
    constexpr int compare(size_type pos1, size_type count1, value_type const *s, size_type count2) const noexcept
    {
        return substr(pos1, count1).compare(string_ref(s, count2));
    }

    size_type find(string_ref v, size_type pos = 0) const noexcept
    {
        if(pos + v.size() > size() || v.empty())
        {
            return npos;
        }
        for(auto f = std::find(cbegin() + pos, cend(), v.front()); size_type(cend() - f) >= v.size(); f = std::find(std::next(f), cend(), v.front()))
        {
            if(string_ref(f, v.size()).compare(v) == 0)
            {
                return f - cbegin();
            }
        }
        return npos;
    }
    size_type find(value_type c, size_type pos = 0) const noexcept
    {
        if(pos >= size())
        {
            return npos;
        }
        auto f = std::find(cbegin() + pos, cend(), c);
        return f == cend() ? npos : f - cbegin();
    }
    constexpr size_type find(value_type const *s, size_type pos, size_type count) const noexcept
    {
        return find(string_ref(s, count), pos);
    }
    constexpr size_type find(value_type const *s, size_type pos = 0) const noexcept
    {
        return find(string_ref(s), pos);
    }

    // constexpr size_type rfind(string_ref const &v, size_type pos = 0) const noexcept;
    // constexpr size_type rfind(value_type c, size_type pos = 0) const noexcept;
    // constexpr size_type rfind(value_type const *s, size_type pos, size_type count) const noexcept;
    // constexpr size_type rfind(value_type const *s, size_type pos = 0) const noexcept;

    // constexpr size_type find_first_of(string_ref const &v, size_type pos = 0) const  noexcept;
    // constexpr size_type find_first_of(value_type c, size_type pos = 0) const  noexcept;
    // constexpr size_type find_first_of(value_type const *s, size_type pos, size_type count) const  noexcept;
    // constexpr size_type find_first_of(value_type const *s, size_type pos = 0) const  noexcept;

    // constexpr size_type find_last_of(string_ref const &v, size_type pos = 0) const  noexcept;
    // constexpr size_type find_last_of(value_type c, size_type pos = 0) const  noexcept;
    // constexpr size_type find_last_of(value_type const *s, size_type pos, size_type count) const  noexcept;
    // constexpr size_type find_last_of(value_type const *s, size_type pos = 0) const  noexcept;

    // constexpr size_type find_first_not_of(string_ref const &v, size_type pos = 0) const  noexcept;
    // constexpr size_type find_first_not_of(value_type c, size_type pos = 0) const  noexcept;
    // constexpr size_type find_first_not_of(value_type const *s, size_type pos, size_type count) const  noexcept;
    // constexpr size_type find_first_not_of(value_type const *s, size_type pos = 0) const  noexcept;

    // constexpr size_type find_last_not_of(string_ref const &v, size_type pos = 0) const  noexcept;
    // constexpr size_type find_last_not_of(value_type c, size_type pos = 0) const  noexcept;
    // constexpr size_type find_last_not_of(value_type const *s, size_type pos, size_type count) const  noexcept;
    // constexpr size_type find_last_not_of(value_type const *s, size_type pos = 0) const  noexcept;

    static constexpr size_type npos = size_type(-1);

private:
    static constexpr int compare_helper(int c, size_type l, size_type r) noexcept
    {
        return c != 0 ? c : l == r ? 0
                        : l < r    ? -1
                                   : 1;
    }

private:
    value_type const *_ptr;
    size_type _len;
};

template<class char_t, class traits_t> constexpr bool operator==(string_ref<char_t, traits_t> left, string_ref<char_t, traits_t> right) noexcept
{
    return left.compare(right) == 0;
}
template<class char_t, class traits_t> constexpr bool operator!=(string_ref<char_t, traits_t> left, string_ref<char_t, traits_t> right) noexcept
{
    return left.compare(right) != 0;
}
template<class char_t, class traits_t> constexpr bool operator<(string_ref<char_t, traits_t> left, string_ref<char_t, traits_t> right) noexcept
{
    return left.compare(right) < 0;
}
template<class char_t, class traits_t> constexpr bool operator>(string_ref<char_t, traits_t> left, string_ref<char_t, traits_t> right) noexcept
{
    return left.compare(right) > 0;
}
template<class char_t, class traits_t> constexpr bool operator<=(string_ref<char_t, traits_t> left, string_ref<char_t, traits_t> right) noexcept
{
    return left.compare(right) <= 0;
}
template<class char_t, class traits_t> constexpr bool operator>=(string_ref<char_t, traits_t> left, string_ref<char_t, traits_t> right) noexcept
{
    return left.compare(right) >= 0;
}
template<class char_t, class traits_t> constexpr bool operator==(char_t const *left, string_ref<char_t, traits_t> right) noexcept
{
    return right.compare(left) == 0;
}
template<class char_t, class traits_t> constexpr bool operator!=(char_t const *left, string_ref<char_t, traits_t> right) noexcept
{
    return right.compare(left) != 0;
}
template<class char_t, class traits_t> constexpr bool operator<(char_t const *left, string_ref<char_t, traits_t> right) noexcept
{
    return right.compare(left) > 0;
}
template<class char_t, class traits_t> constexpr bool operator>(char_t const *left, string_ref<char_t, traits_t> right) noexcept
{
    return right.compare(left) < 0;
}
template<class char_t, class traits_t> constexpr bool operator<=(char_t const *left, string_ref<char_t, traits_t> right) noexcept
{
    return right.compare(left) >= 0;
}
template<class char_t, class traits_t> constexpr bool operator>=(char_t const *left, string_ref<char_t, traits_t> right) noexcept
{
    return right.compare(left) <= 0;
}
template<class char_t, class traits_t> constexpr bool operator==(string_ref<char_t, traits_t> left, char_t const *right) noexcept
{
    return left.compare(right) == 0;
}
template<class char_t, class traits_t> constexpr bool operator!=(string_ref<char_t, traits_t> left, char_t const *right) noexcept
{
    return left.compare(right) != 0;
}
template<class char_t, class traits_t> constexpr bool operator<(string_ref<char_t, traits_t> left, char_t const *right) noexcept
{
    return left.compare(right) < 0;
}
template<class char_t, class traits_t> constexpr bool operator>(string_ref<char_t, traits_t> left, char_t const *right) noexcept
{
    return left.compare(right) > 0;
}
template<class char_t, class traits_t> constexpr bool operator<=(string_ref<char_t, traits_t> left, char_t const *right) noexcept
{
    return left.compare(right) <= 0;
}
template<class char_t, class traits_t> constexpr bool operator>=(string_ref<char_t, traits_t> left, char_t const *right) noexcept
{
    return left.compare(right) >= 0;
}
template<class char_t, class traits_t, class allocator_t> bool operator==(std::basic_string<char_t, traits_t, allocator_t> const &left, string_ref<char_t, traits_t> right) noexcept
{
    return right.compare(string_ref<char_t, traits_t>(left)) == 0;
}
template<class char_t, class traits_t, class allocator_t> bool operator!=(std::basic_string<char_t, traits_t, allocator_t> const &left, string_ref<char_t, traits_t> right) noexcept
{
    return right.compare(string_ref<char_t, traits_t>(left)) != 0;
}
template<class char_t, class traits_t, class allocator_t> bool operator<(std::basic_string<char_t, traits_t, allocator_t> const &left, string_ref<char_t, traits_t> right) noexcept
{
    return right.compare(string_ref<char_t, traits_t>(left)) > 0;
}
template<class char_t, class traits_t, class allocator_t> bool operator>(std::basic_string<char_t, traits_t, allocator_t> const &left, string_ref<char_t, traits_t> right) noexcept
{
    return right.compare(string_ref<char_t, traits_t>(left)) < 0;
}
template<class char_t, class traits_t, class allocator_t> bool operator<=(std::basic_string<char_t, traits_t, allocator_t> const &left, string_ref<char_t, traits_t> right) noexcept
{
    return right.compare(string_ref<char_t, traits_t>(left)) >= 0;
}
template<class char_t, class traits_t, class allocator_t> bool operator>=(std::basic_string<char_t, traits_t, allocator_t> const &left, string_ref<char_t, traits_t> right) noexcept
{
    return right.compare(string_ref<char_t, traits_t>(left)) <= 0;
}
template<class char_t, class traits_t, class allocator_t> bool operator==(string_ref<char_t, traits_t> left, std::basic_string<char_t, traits_t, allocator_t> const &right) noexcept
{
    return left.compare(string_ref<char_t, traits_t>(right)) == 0;
}
template<class char_t, class traits_t, class allocator_t> bool operator!=(string_ref<char_t, traits_t> left, std::basic_string<char_t, traits_t, allocator_t> const &right) noexcept
{
    return left.compare(string_ref<char_t, traits_t>(right)) != 0;
}
template<class char_t, class traits_t, class allocator_t> bool operator<(string_ref<char_t, traits_t> left, std::basic_string<char_t, traits_t, allocator_t> const &right) noexcept
{
    return left.compare(string_ref<char_t, traits_t>(right)) < 0;
}
template<class char_t, class traits_t, class allocator_t> bool operator>(string_ref<char_t, traits_t> left, std::basic_string<char_t, traits_t, allocator_t> const &right) noexcept
{
    return left.compare(string_ref<char_t, traits_t>(right)) > 0;
}
template<class char_t, class traits_t, class allocator_t> bool operator<=(string_ref<char_t, traits_t> left, std::basic_string<char_t, traits_t, allocator_t> const &right) noexcept
{
    return left.compare(string_ref<char_t, traits_t>(right)) <= 0;
}
template<class char_t, class traits_t, class allocator_t> bool operator>=(string_ref<char_t, traits_t> left, std::basic_string<char_t, traits_t, allocator_t> const &right) noexcept
{
    return left.compare(string_ref<char_t, traits_t>(right)) >= 0;
}
#endif // __cplusplus >= 201703L

template<class char_t> bool to_value_is_space_(char_t c)
{
    return c == char_t(' ') || c == char_t('\t') || c == char_t('\n') || c == char_t('\r') || c == char_t('\f') || c == char_t('\v');
}
// Aligns with std::stoi/std::stod: throws std::invalid_argument when no
// conversion can be performed and std::out_of_range when the parsed value does
// not fit into to_t. Leading whitespace and an optional sign are skipped, then
// the actual parsing is delegated to string_to_integer / string_to_real so the
// conversion logic lives in a single place.
template<class to_t, class char_t> typename std::enable_if<std::is_integral<to_t>::value, to_t>::type to_value_checked_(char_t const *s, size_t l)
{
    char_t const *p = s;
    char_t const *end = s + l;
    while(p != end && to_value_is_space_(*p))
        ++p;
    // string_to_integer / std::from_chars accept a leading '-' but reject '+';
    // strip an optional leading '+' so "+123" keeps working like std::stoi.
    if(p != end && *p == char_t('+'))
        ++p;
    std::errc ec = std::errc();
    to_t value = string_to_integer<char_t, to_t>(p, size_t(end - p), &ec);
    if(ec == std::errc::result_out_of_range)
        throw std::out_of_range("to_value: out of range");
    if(ec != std::errc())
        throw std::invalid_argument("to_value: no conversion");
    return value;
}
template<class to_t, class char_t> typename std::enable_if<std::is_floating_point<to_t>::value, to_t>::type to_value_checked_(char_t const *s, size_t l)
{
    char_t const *p = s;
    char_t const *end = s + l;
    while(p != end && to_value_is_space_(*p))
        ++p;
    // string_to_real / std::from_chars accept a leading '-' but reject '+'.
    if(p != end && *p == char_t('+'))
        ++p;
    std::errc ec = std::errc();
    to_t value = string_to_real<char_t, to_t>(p, size_t(end - p), &ec);
    if(ec == std::errc::result_out_of_range)
        throw std::out_of_range("to_value: out of range");
    if(ec != std::errc())
        throw std::invalid_argument("to_value: no conversion");
    return value;
}
#if __cplusplus >= 201703L
template<class to_t, class char_t, class traits_t>
to_t to_value(std::basic_string_view<char_t, traits_t> sv)
{
    return to_value_checked_<to_t>(sv.data(), sv.size());
}
template<class char_t, class traits_t>
std::basic_string<char_t, traits_t> to_string(std::basic_string_view<char_t, traits_t> sv)
{
    return std::basic_string<char_t, traits_t>(sv);
}
#else
template<class to_t, class char_t, class traits_t>
typename std::enable_if<std::is_integral<to_t>::value, to_t>::type
to_value(string_ref<char_t, traits_t> sv)
{
    return to_value_checked_<to_t>(sv.data(), sv.size());
}
template<class to_t, class char_t, class traits_t>
typename std::enable_if<std::is_floating_point<to_t>::value, to_t>::type
to_value(string_ref<char_t, traits_t> sv)
{
    return to_value_checked_<to_t>(sv.data(), sv.size());
}
template<class char_t, class traits_t>
std::basic_string<char_t, traits_t> to_string(string_ref<char_t, traits_t> sv)
{
    return std::basic_string<char_t, traits_t>(sv.data(), sv.size());
}
#endif

template<class char_t, class traits_t = std::char_traits<char_t>, class string_t = string_ref<char_t, traits_t>> struct split_iterator_finder_char
{
public:
    typedef char_t char_type;
    typedef traits_t traits_type;
    typedef string_t string_type;
    typedef typename string_type::size_type size_type;

    split_iterator_finder_char() : _find()
    {
    }
    split_iterator_finder_char(split_iterator_finder_char const &) = default;
    split_iterator_finder_char(char_t find) : _find(find)
    {
    }
    size_type run(string_type ref, size_type pos) const
    {
        return ref.find(_find, pos);
    }
    size_type size() const
    {
        return 1;
    }

private:
    char_t _find;
};
template<class char_t, class traits_t = std::char_traits<char_t>, class string_t = string_ref<char_t, traits_t>> struct split_iterator_finder_string
{
public:
    typedef char_t char_type;
    typedef traits_t traits_type;
    typedef string_t string_type;
    typedef typename string_type::size_type size_type;

    split_iterator_finder_string() : _find()
    {
    }
    split_iterator_finder_string(split_iterator_finder_string const &) = default;
    split_iterator_finder_string(string_type find) : _find(find)
    {
    }
    size_type run(string_type ref, size_type pos) const
    {
        return ref.find(_find, pos);
    }
    size_type size() const
    {
        return _find.size();
    }

private:
    string_type _find;
};
template<class char_t, class traits_t = std::char_traits<char_t>, class string_t = string_ref<char_t, traits_t>> struct split_iterator_finder_any_of
{
public:
    typedef char_t char_type;
    typedef traits_t traits_type;
    typedef string_t string_type;
    typedef typename string_type::size_type size_type;

    split_iterator_finder_any_of() : _find()
    {
    }
    split_iterator_finder_any_of(split_iterator_finder_any_of const &) = default;
    split_iterator_finder_any_of(string_type find) : _find(find)
    {
    }
    size_type run(string_type ref, size_type pos) const
    {
        for(; pos < ref.size(); ++pos)
        {
            if(std::find(_find.cbegin(), _find.cend(), ref[pos]) != _find.cend())
            {
                return pos;
            }
        }
        return string_type::npos;
    }
    size_type size() const
    {
        return 1;
    }

private:
    string_type _find;
};
template<class finder_t> class split_container
{
public:
    typedef typename finder_t::string_type value_type, string_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef value_type const &reference;
    typedef value_type const &const_reference;
    typedef value_type const *pointer;
    typedef value_type const *const_pointer;

    class iterator
    {
    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef typename split_container::value_type value_type;
        typedef typename split_container::difference_type difference_type;
        typedef typename split_container::reference reference;
        typedef typename split_container::pointer pointer;

    public:
        iterator(split_container const *_self) : self(_self)
        {
            self->find_first(this);
        }
        iterator() : self(), pos(), current()
        {
        }
        iterator(iterator const &) = default;
        iterator &operator++()
        {
            self->find_next(this);
            return *this;
        }
        iterator operator++(int)
        {
            iterator save(*this);
            ++*this;
            return save;
        }
        reference operator*() const
        {
            return current;
        }
        pointer operator->() const
        {
            return &current;
        }
        bool operator==(iterator const &other) const
        {
            return self == other.self && pos == other.pos && current == other.current;
        }
        bool operator!=(iterator const &other) const
        {
            return !(*this == other);
        }

    private:
        friend class split_container;
        split_container const *self;
        size_type pos;
        string_type current;
    };
    typedef iterator const_iterator;

private:
    template<class unuse, class T> struct fill_value_t
    {
        int operator()(iterator &it, T &value, bool &error)
        {
            if(it.self != nullptr)
            {
                value = to_value<T>(*it);
                ++it;
            }
            else
            {
                error = true;
            }
            return 0;
        }
    };
    template<class unuse> struct fill_value_t<unuse, string_type>
    {
        int operator()(iterator &it, string_type &value, bool &error)
        {
            if(it.self != nullptr)
            {
                value = *it;
                ++it;
            }
            else
            {
                error = true;
            }
            return 0;
        }
    };
    template<class unuse, class allocator_t> struct fill_value_t<unuse, std::basic_string<typename string_type::value_type, typename string_type::traits_type, allocator_t>>
    {
        int operator()(iterator &it, std::basic_string<typename string_type::value_type, typename string_type::traits_type, allocator_t> &value, bool &error)
        {
            if(it.self != nullptr)
            {
                value = *it;
                ++it;
            }
            else
            {
                error = true;
            }
            return 0;
        }
    };

public:
    typedef std::pair<string_type, string_type> pair_ss_t;

    split_container() : _size(0), _ref(), _finder()
    {
    }
    split_container(split_container const &) = default;
    split_container(string_type ref, finder_t &&finder) : _size(), _ref(ref), _finder(std::move(finder))
    {
    }
    split_container &operator=(split_container const &) = default;

    void find_first(iterator *it) const
    {
        it->current = _ref.substr(0, _finder.run(_ref, 0));
        it->pos = it->current.size();
    }
    void find_next(iterator *it) const
    {
        if(it->pos == _ref.size())
        {
            it->self = nullptr;
            it->pos = 0;
            it->current = {};
        }
        else
        {
            it->pos += _finder.size();
            it->current = _ref.substr(it->pos, _finder.run(_ref, it->pos) - it->pos);
            it->pos += it->current.size();
        }
    }

    ZZZ_LIB_NODISCARD iterator begin() const
    {
        return iterator(this);
    }
    ZZZ_LIB_NODISCARD iterator end() const
    {
        return iterator();
    }
    ZZZ_LIB_NODISCARD iterator cbegin() const
    {
        return iterator(this);
    }
    ZZZ_LIB_NODISCARD iterator cend() const
    {
        return iterator();
    }

    ZZZ_LIB_NODISCARD pair_ss_t split2() const
    {
        size_type pos = _finder.run(_ref, 0);
        if(pos == string_type::npos)
        {
            return { _ref, string_type() };
        }
        return {
            _ref.substr(0, pos), _ref.substr(pos + _finder.size())
        };
    }

    template<class... args_t> bool fill(args_t &...value)
    {
        bool error = false;
        iterator it = begin();
        std::initializer_list<int>({ fill_value_t<void, args_t>()(it, value, error)... });
        return !error;
    }

    ZZZ_LIB_NODISCARD size_type size() const
    {
        if(_size == 0 && !_ref.empty())
        {
            size_type pos = _finder.run(_ref, 0);
            while(pos < _ref.size())
            {
                ++_size;
                pos = _finder.run(_ref, pos + _finder.size());
            }
            ++_size;
        }
        return _size;
    }

    // Complexity: O(N) in the number of split fields; cache results when using repeated index access.
    ZZZ_LIB_NODISCARD string_type operator[](size_type index)
    {
        if(index == 0)
        {
            return _ref.substr(0, _finder.run(_ref, 0));
        }
        size_type count = 0, pos = _finder.run(_ref, 0);
        while(pos < _ref.size())
        {
            ++count;
            if(count == index)
            {
                pos += _finder.size();
                return _ref.substr(pos, _finder.run(_ref, pos) - pos);
            }
            else
            {
                pos = _finder.run(_ref, pos + _finder.size());
            }
        }
        return string_type();
    }

private:
    mutable size_type _size;
    string_type _ref;
    finder_t _finder;
};

template<class char_t, class traits_t, class allocator_t> split_container<split_iterator_finder_char<char_t, traits_t>> make_split(std::basic_string<char_t, traits_t, allocator_t> const &str, char_t find)
{
    return split_container<split_iterator_finder_char<char_t, traits_t>>(str, { find });
};
template<class char_t, class traits_t> split_container<split_iterator_finder_char<char_t, traits_t>> make_split(string_ref<char_t, traits_t> str, char_t find)
{
    return split_container<split_iterator_finder_char<char_t, traits_t>>(str, { find });
};

template<class char_t, class traits_t, class allocator_t> split_container<split_iterator_finder_string<char_t, traits_t>> make_split(std::basic_string<char_t, traits_t, allocator_t> const &str, char_t const *find)
{
    return split_container<split_iterator_finder_string<char_t, traits_t>>(str, { find });
};
template<class char_t, class traits_t> split_container<split_iterator_finder_string<char_t, traits_t>> make_split(string_ref<char_t, traits_t> str, char_t const *find)
{
    return split_container<split_iterator_finder_string<char_t, traits_t>>(str, { find });
};

template<class char_t, class traits_t, class allocator_t> split_container<split_iterator_finder_string<char_t, traits_t>> make_split(std::basic_string<char_t, traits_t, allocator_t> const &str, std::basic_string<char_t, traits_t, allocator_t> const &find)
{
    return split_container<split_iterator_finder_string<char_t, traits_t>>(str, { find });
};
template<class char_t, class traits_t, class allocator_t> split_container<split_iterator_finder_string<char_t, traits_t>> make_split(string_ref<char_t, traits_t> str, std::basic_string<char_t, traits_t, allocator_t> const &find)
{
    return split_container<split_iterator_finder_string<char_t, traits_t>>(str, { find });
};

template<class char_t, class traits_t, class allocator_t> split_container<split_iterator_finder_any_of<char_t, traits_t>> make_split_any_of(std::basic_string<char_t, traits_t, allocator_t> const &str, char_t const *find)
{
    return split_container<split_iterator_finder_any_of<char_t, traits_t>>(str, { find });
};
template<class char_t, class traits_t> split_container<split_iterator_finder_any_of<char_t, traits_t>> make_split_any_of(string_ref<char_t, traits_t> str, char_t const *find)
{
    return split_container<split_iterator_finder_any_of<char_t, traits_t>>(str, { find });
};

template<class char_t, class traits_t, class allocator_t> split_container<split_iterator_finder_any_of<char_t, traits_t>> make_split_any_of(std::basic_string<char_t, traits_t, allocator_t> const &str, std::basic_string<char_t, traits_t, allocator_t> const &find)
{
    return split_container<split_iterator_finder_any_of<char_t, traits_t>>(str, { find });
};
template<class char_t, class traits_t, class allocator_t> split_container<split_iterator_finder_any_of<char_t, traits_t>> make_split_any_of(string_ref<char_t, traits_t> str, std::basic_string<char_t, traits_t, allocator_t> const &find)
{
    return split_container<split_iterator_finder_any_of<char_t, traits_t>>(str, { find });
};
