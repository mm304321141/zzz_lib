
#pragma once

#include <cstdint>
#include <algorithm>
#include <utility>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <cstring>
#include <string>
#include <cmath>
#include <iterator>
#include <memory>
//#include <charconv>

template<class char_t, class integer_t> integer_t string_to_integer(char_t const *s, size_t l)
{
    integer_t result = 0;
    if(l == 0)
    {
        return result;
    }
    size_t nc, neg = s[0] == '-' ? 1 : 0;
    for(nc = neg; nc != l && isdigit(s[nc]); ++nc)
        ;
    for(size_t i = neg; i != nc; ++i)
    {
        switch(nc - i)
        {
            case 19: result += (s[i] - '0') * integer_t(1000000000000000000LL); break;
            case 18: result += (s[i] - '0') * integer_t(100000000000000000LL); break;
            case 17: result += (s[i] - '0') * integer_t(10000000000000000LL); break;
            case 16: result += (s[i] - '0') * integer_t(1000000000000000LL); break;
            case 15: result += (s[i] - '0') * integer_t(100000000000000LL); break;
            case 14: result += (s[i] - '0') * integer_t(10000000000000LL); break;
            case 13: result += (s[i] - '0') * integer_t(1000000000000LL); break;
            case 12: result += (s[i] - '0') * integer_t(100000000000LL); break;
            case 11: result += (s[i] - '0') * integer_t(10000000000LL); break;
            case 10: result += (s[i] - '0') * integer_t(1000000000LL); break;
            case 9: result += (s[i] - '0') * integer_t(100000000); break;
            case 8: result += (s[i] - '0') * integer_t(10000000); break;
            case 7: result += (s[i] - '0') * integer_t(1000000); break;
            case 6: result += (s[i] - '0') * integer_t(100000); break;
            case 5: result += (s[i] - '0') * integer_t(10000); break;
            case 4: result += (s[i] - '0') * integer_t(1000); break;
            case 3: result += (s[i] - '0') * integer_t(100); break;
            case 2: result += (s[i] - '0') * integer_t(10); break;
            case 1: result += (s[i] - '0'); break;
            case 0: break;
            default: break;
        }
    }
    return neg && !std::is_unsigned<integer_t>::value ? result * -1 : result;
}

template<class char_t, class real_t> real_t string_to_real(char_t const *s, size_t l)
{
    double result = 0.0;
    if(l == 0)
    {
        return real_t(result);
    }
    std::intptr_t i, nc, neg = s[0] == '-' ? 1 : 0;
    for(nc = neg; size_t(nc) != l && (isdigit(s[nc]) && s[nc] != '.'); ++nc)
        ;
    for(i = neg; size_t(i) != l && (isdigit(s[i]) || s[i] == '.'); ++i)
    {
        switch(nc - i)
        {
            case 19: result += (s[i] - '0') * 1000000000000000000.0; break;
            case 18: result += (s[i] - '0') * 100000000000000000.0; break;
            case 17: result += (s[i] - '0') * 10000000000000000.0; break;
            case 16: result += (s[i] - '0') * 1000000000000000.0; break;
            case 15: result += (s[i] - '0') * 100000000000000.0; break;
            case 14: result += (s[i] - '0') * 10000000000000.0; break;
            case 13: result += (s[i] - '0') * 1000000000000.0; break;
            case 12: result += (s[i] - '0') * 100000000000.0; break;
            case 11: result += (s[i] - '0') * 10000000000.0; break;
            case 10: result += (s[i] - '0') * 1000000000.0; break;
            case 9: result += (s[i] - '0') * 100000000.0; break;
            case 8: result += (s[i] - '0') * 10000000; break;
            case 7: result += (s[i] - '0') * 1000000; break;
            case 6: result += (s[i] - '0') * 100000; break;
            case 5: result += (s[i] - '0') * 10000; break;
            case 4: result += (s[i] - '0') * 1000; break;
            case 3: result += (s[i] - '0') * 100; break;
            case 2: result += (s[i] - '0') * 10; break;
            case 1: result += (s[i] - '0'); break;
            case 0: break;
            case -1: result += (s[i] - '0') * 0.1; break;
            case -2: result += (s[i] - '0') * 0.01; break;
            case -3: result += (s[i] - '0') * 0.001; break;
            case -4: result += (s[i] - '0') * 0.0001; break;
            case -5: result += (s[i] - '0') * 0.00001; break;
            case -6: result += (s[i] - '0') * 0.000001; break;
            case -7: result += (s[i] - '0') * 0.0000001; break;
            case -8: result += (s[i] - '0') * 0.00000001; break;
            case -9: result += (s[i] - '0') * 0.000000001; break;
            case -10: result += (s[i] - '0') * 0.0000000001; break;
            case -11: result += (s[i] - '0') * 0.00000000001; break;
            case -12: result += (s[i] - '0') * 0.000000000001; break;
            case -13: result += (s[i] - '0') * 0.0000000000001; break;
            case -14: result += (s[i] - '0') * 0.00000000000001; break;
            case -15: result += (s[i] - '0') * 0.000000000000001; break;
            case -16: result += (s[i] - '0') * 0.0000000000000001; break;
            case -17: result += (s[i] - '0') * 0.00000000000000001; break;
            case -18: result += (s[i] - '0') * 0.000000000000000001; break;
            case -19: result += (s[i] - '0') * 0.0000000000000000001; break;
            default:
                result += (s[i] - '0') * std::pow(10, nc - i > 0 ? nc - i - 1 : nc - i);
                break;
        }
    }
    if(size_t(i) != l && s[i] == 'e')
    {
        result *= std::pow(10, string_to_integer<char_t, std::intptr_t>(s + i + 1, l - i - 1));
    }
    return real_t(neg ? result * -1.0 : result);
}

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

    string_ref &operator = (string_ref const &) noexcept = default;
    
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
        return _len;
    }

    void clear() noexcept
    {
        _len = 0;
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

    template<class allocator_t = std::allocator<value_type>> std::basic_string<char_t, traits_t, allocator_t> to_string(allocator_t const &a = allocator_t()) const noexcept
    {
        return std::basic_string<char_t, traits_t, allocator_t>(data(), length(), a);
    }
    template<class allocator_t> operator std::basic_string<char_t, traits_t, allocator_t>() const noexcept
    {
        return std::basic_string<char_t, traits_t, allocator_t>(data(), length());
    }
    template<class to_t> typename std::enable_if<std::is_integral<to_t>::value, to_t>::type to_value() const noexcept
    {
        return string_to_integer<value_type, to_t>(_ptr, _len);
    }
    template<class to_t> typename std::enable_if<std::is_floating_point<to_t>::value, to_t>::type to_value() const noexcept
    {
        return string_to_real<value_type, to_t>(_ptr, _len);
    }

    size_type copy(value_type *dest, size_type count, size_type pos = 0) const noexcept
    {
        if(pos >= size())
        {
            return 0;
        }
        return std::uninitialized_copy_n(_ptr, std::min(count, _len), dest) - dest;
    }
    constexpr string_ref substr(size_type pos = 0, size_type count = npos) const
    {
        return pos > size() ? throw std::out_of_range("string_ref out of range") : string_ref(_ptr + pos, std::min(count, size() - pos));
    }
    constexpr string_ref concat(string_ref v) const noexcept
    {
        return concat_helper(v, std::make_index_sequence<_len + v._len>());
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

    //constexpr size_type rfind(string_ref const &v, size_type pos = 0) const noexcept;
    //constexpr size_type rfind(value_type c, size_type pos = 0) const noexcept;
    //constexpr size_type rfind(value_type const *s, size_type pos, size_type count) const noexcept;
    //constexpr size_type rfind(value_type const *s, size_type pos = 0) const noexcept;

    //constexpr size_type find_first_of(string_ref const &v, size_type pos = 0) const  noexcept;
    //constexpr size_type find_first_of(value_type c, size_type pos = 0) const  noexcept;
    //constexpr size_type find_first_of(value_type const *s, size_type pos, size_type count) const  noexcept;
    //constexpr size_type find_first_of(value_type const *s, size_type pos = 0) const  noexcept;

    //constexpr size_type find_last_of(string_ref const &v, size_type pos = 0) const  noexcept;
    //constexpr size_type find_last_of(value_type c, size_type pos = 0) const  noexcept;
    //constexpr size_type find_last_of(value_type const *s, size_type pos, size_type count) const  noexcept;
    //constexpr size_type find_last_of(value_type const *s, size_type pos = 0) const  noexcept;

    //constexpr size_type find_first_not_of(string_ref const &v, size_type pos = 0) const  noexcept;
    //constexpr size_type find_first_not_of(value_type c, size_type pos = 0) const  noexcept;
    //constexpr size_type find_first_not_of(value_type const *s, size_type pos, size_type count) const  noexcept;
    //constexpr size_type find_first_not_of(value_type const *s, size_type pos = 0) const  noexcept;

    //constexpr size_type find_last_not_of(string_ref const &v, size_type pos = 0) const  noexcept;
    //constexpr size_type find_last_not_of(value_type c, size_type pos = 0) const  noexcept;
    //constexpr size_type find_last_not_of(value_type const *s, size_type pos, size_type count) const  noexcept;
    //constexpr size_type find_last_not_of(value_type const *s, size_type pos = 0) const  noexcept;

    static constexpr size_type npos = size_type(-1);

private:
    static constexpr int compare_helper(int c, size_type l, size_type r) noexcept
    {
        return c != 0 ? c : l == r ? 0 : l < r ? -1 : 1;
    }

private:
    value_type const *_ptr;
    size_type _len;
};

template<class char_t, class traits_t> constexpr bool operator == (string_ref<char_t, traits_t> left, string_ref<char_t, traits_t> right) noexcept { return left.compare(right) == 0; }
template<class char_t, class traits_t> constexpr bool operator != (string_ref<char_t, traits_t> left, string_ref<char_t, traits_t> right) noexcept { return left.compare(right) != 0; }
template<class char_t, class traits_t> constexpr bool operator <  (string_ref<char_t, traits_t> left, string_ref<char_t, traits_t> right) noexcept { return left.compare(right) <  0; }
template<class char_t, class traits_t> constexpr bool operator >  (string_ref<char_t, traits_t> left, string_ref<char_t, traits_t> right) noexcept { return left.compare(right) >  0; }
template<class char_t, class traits_t> constexpr bool operator <= (string_ref<char_t, traits_t> left, string_ref<char_t, traits_t> right) noexcept { return left.compare(right) <= 0; }
template<class char_t, class traits_t> constexpr bool operator >= (string_ref<char_t, traits_t> left, string_ref<char_t, traits_t> right) noexcept { return left.compare(right) >= 0; }
template<class char_t, class traits_t> constexpr bool operator == (char_t const *left, string_ref<char_t, traits_t> right) noexcept { return right.compare(left) == 0; }
template<class char_t, class traits_t> constexpr bool operator != (char_t const *left, string_ref<char_t, traits_t> right) noexcept { return right.compare(left) != 0; }
template<class char_t, class traits_t> constexpr bool operator <  (char_t const *left, string_ref<char_t, traits_t> right) noexcept { return right.compare(left) >  0; }
template<class char_t, class traits_t> constexpr bool operator >  (char_t const *left, string_ref<char_t, traits_t> right) noexcept { return right.compare(left) <  0; }
template<class char_t, class traits_t> constexpr bool operator <= (char_t const *left, string_ref<char_t, traits_t> right) noexcept { return right.compare(left) >= 0; }
template<class char_t, class traits_t> constexpr bool operator >= (char_t const *left, string_ref<char_t, traits_t> right) noexcept { return right.compare(left) <= 0; }
template<class char_t, class traits_t> constexpr bool operator == (string_ref<char_t, traits_t> left, char_t const *right) noexcept { return left.compare(right) == 0; }
template<class char_t, class traits_t> constexpr bool operator != (string_ref<char_t, traits_t> left, char_t const *right) noexcept { return left.compare(right) != 0; }
template<class char_t, class traits_t> constexpr bool operator <  (string_ref<char_t, traits_t> left, char_t const *right) noexcept { return left.compare(right) <  0; }
template<class char_t, class traits_t> constexpr bool operator >  (string_ref<char_t, traits_t> left, char_t const *right) noexcept { return left.compare(right) >  0; }
template<class char_t, class traits_t> constexpr bool operator <= (string_ref<char_t, traits_t> left, char_t const *right) noexcept { return left.compare(right) <= 0; }
template<class char_t, class traits_t> constexpr bool operator >= (string_ref<char_t, traits_t> left, char_t const *right) noexcept { return left.compare(right) >= 0; }
template<class char_t, class traits_t, class allocator_t> bool operator == (std::basic_string<char_t, traits_t, allocator_t> const &left, string_ref<char_t, traits_t> right) noexcept { return right.compare(string_ref<char_t, traits_t>(left)) == 0; }
template<class char_t, class traits_t, class allocator_t> bool operator != (std::basic_string<char_t, traits_t, allocator_t> const &left, string_ref<char_t, traits_t> right) noexcept { return right.compare(string_ref<char_t, traits_t>(left)) != 0; }
template<class char_t, class traits_t, class allocator_t> bool operator <  (std::basic_string<char_t, traits_t, allocator_t> const &left, string_ref<char_t, traits_t> right) noexcept { return right.compare(string_ref<char_t, traits_t>(left)) >  0; }
template<class char_t, class traits_t, class allocator_t> bool operator >  (std::basic_string<char_t, traits_t, allocator_t> const &left, string_ref<char_t, traits_t> right) noexcept { return right.compare(string_ref<char_t, traits_t>(left)) <  0; }
template<class char_t, class traits_t, class allocator_t> bool operator <= (std::basic_string<char_t, traits_t, allocator_t> const &left, string_ref<char_t, traits_t> right) noexcept { return right.compare(string_ref<char_t, traits_t>(left)) >= 0; }
template<class char_t, class traits_t, class allocator_t> bool operator >= (std::basic_string<char_t, traits_t, allocator_t> const &left, string_ref<char_t, traits_t> right) noexcept { return right.compare(string_ref<char_t, traits_t>(left)) <= 0; }
template<class char_t, class traits_t, class allocator_t> bool operator == (string_ref<char_t, traits_t> left, std::basic_string<char_t, traits_t, allocator_t> const &right) noexcept { return left.compare(string_ref<char_t, traits_t>(right)) == 0; }
template<class char_t, class traits_t, class allocator_t> bool operator != (string_ref<char_t, traits_t> left, std::basic_string<char_t, traits_t, allocator_t> const &right) noexcept { return left.compare(string_ref<char_t, traits_t>(right)) != 0; }
template<class char_t, class traits_t, class allocator_t> bool operator <  (string_ref<char_t, traits_t> left, std::basic_string<char_t, traits_t, allocator_t> const &right) noexcept { return left.compare(string_ref<char_t, traits_t>(right)) <  0; }
template<class char_t, class traits_t, class allocator_t> bool operator >  (string_ref<char_t, traits_t> left, std::basic_string<char_t, traits_t, allocator_t> const &right) noexcept { return left.compare(string_ref<char_t, traits_t>(right)) >  0; }
template<class char_t, class traits_t, class allocator_t> bool operator <= (string_ref<char_t, traits_t> left, std::basic_string<char_t, traits_t, allocator_t> const &right) noexcept { return left.compare(string_ref<char_t, traits_t>(right)) <= 0; }
template<class char_t, class traits_t, class allocator_t> bool operator >= (string_ref<char_t, traits_t> left, std::basic_string<char_t, traits_t, allocator_t> const &right) noexcept { return left.compare(string_ref<char_t, traits_t>(right)) >= 0; }

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
        reference operator *() const
        {
            return current;
        }
        pointer operator->() const
        {
            return &current;
        }
        bool operator == (iterator const &other) const
        {
            return self == other.self && pos == other.pos && current == other.current;
        }
        bool operator != (iterator const &other) const
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
                value = it->template to_value<T>();
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
    
    split_container() : _size(1), _ref(), _finder()
    {
    }
    split_container(split_container const &) = default;
    split_container(string_type ref, finder_t &&finder) : _size(), _ref(ref), _finder(std::move(finder))
    {
    }
    split_container &operator = (split_container const &) = default;
    
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
            it->current.clear();
        }
        else
        {
            it->pos += _finder.size();
            it->current = _ref.substr(it->pos, _finder.run(_ref, it->pos) - it->pos);
            it->pos += it->current.size();
        }
    }
    
    iterator begin() const
    {
        return iterator(this);
    }
    iterator end() const
    {
        return iterator();
    }
    iterator cbegin() const
    {
        return iterator(this);
    }
    iterator cend() const
    {
        return iterator();
    }

    pair_ss_t split2() const
    {
        size_type pos = _finder.run(_ref, 0);
        return
        {
            _ref.substr(0, pos), _ref.substr(pos + _finder.size())
        };
    }

    template<class ...args_t> bool fill(args_t &...value)
    {
        bool error = false;
        iterator it = begin();
        std::initializer_list<int>({fill_value_t<void, args_t>()(it, value, error)...});
        return !error;
    }

    size_type size() const
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

    string_type operator[](size_type index)
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
    return split_container<split_iterator_finder_char<char_t, traits_t>>(str, {find});
};
template<class char_t, class traits_t> split_container<split_iterator_finder_char<char_t, traits_t>> make_split(string_ref<char_t, traits_t> str, char_t find)
{
    return split_container<split_iterator_finder_char<char_t, traits_t>>(str, {find});
};

template<class char_t, class traits_t, class allocator_t> split_container<split_iterator_finder_string<char_t, traits_t>> make_split(std::basic_string<char_t, traits_t, allocator_t> const &str, char_t const *find)
{
    return split_container<split_iterator_finder_string<char_t, traits_t>>(str, {find});
};
template<class char_t, class traits_t> split_container<split_iterator_finder_string<char_t, traits_t>> make_split(string_ref<char_t, traits_t> str, char_t const *find)
{
    return split_container<split_iterator_finder_string<char_t, traits_t>>(str, {find});
};

template<class char_t, class traits_t, class allocator_t> split_container<split_iterator_finder_string<char_t, traits_t>> make_split(std::basic_string<char_t, traits_t, allocator_t> const &str, std::basic_string<char_t, traits_t, allocator_t> const &find)
{
    return split_container<split_iterator_finder_string<char_t, traits_t>>(str, {find});
};
template<class char_t, class traits_t, class allocator_t> split_container<split_iterator_finder_string<char_t, traits_t>> make_split(string_ref<char_t, traits_t> str, std::basic_string<char_t, traits_t, allocator_t> const &find)
{
    return split_container<split_iterator_finder_string<char_t, traits_t>>(str, {find});
};

template<class char_t, class traits_t, class allocator_t> split_container<split_iterator_finder_any_of<char_t, traits_t>> make_split_any_of(std::basic_string<char_t, traits_t, allocator_t> const &str, char_t const *find)
{
    return split_container<split_iterator_finder_any_of<char_t, traits_t>>(str, {find});
};
template<class char_t, class traits_t> split_container<split_iterator_finder_any_of<char_t, traits_t>> make_split_any_of(string_ref<char_t, traits_t> str, char_t const *find)
{
    return split_container<split_iterator_finder_any_of<char_t, traits_t>>(str, {find});
};

template<class char_t, class traits_t, class allocator_t> split_container<split_iterator_finder_any_of<char_t, traits_t>> make_split_any_of(std::basic_string<char_t, traits_t, allocator_t> const &str, std::basic_string<char_t, traits_t, allocator_t> const &find)
{
    return split_container<split_iterator_finder_any_of<char_t, traits_t>>(str, {find});
};
template<class char_t, class traits_t, class allocator_t> split_container<split_iterator_finder_any_of<char_t, traits_t>> make_split_any_of(string_ref<char_t, traits_t> str, std::basic_string<char_t, traits_t, allocator_t> const &find)
{
    return split_container<split_iterator_finder_any_of<char_t, traits_t>>(str, {find});
};

