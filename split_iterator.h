
#pragma once

#include <cstdint>
#include <algorithm>
#include <utility>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <cstring>
#include <string>
#include <iterator>

template<class char_t, class integer_t> integer_t string_to_integer(char_t const *s, size_t l)
{
    integer_t result = 0;
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
    return neg && std::is_unsigned<integer_t>::value ? result * -1 : result;
}

template<class char_t, class real_t> real_t string_to_real(char_t const *s, size_t l)
{
    double result = 0.0;
    std::intptr_t nc, neg = s[0] == '-' ? 1 : 0;
    for(nc = neg; nc != std::intptr_t(l) && (isdigit(s[nc]) && s[nc] != '.'); ++nc)
        ;
    for(std::intptr_t i = neg; i != std::intptr_t(l) && (isdigit(s[i]) || s[i] == '.'); ++i)
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
            default: break;
        }
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
    constexpr string_ref() : _ptr(), _len()
    {
    }
    constexpr string_ref(string_ref const &other) = default;
    template<class allocator_t> string_ref(std::basic_string<char_t, traits_t, allocator_t> const &str) : _ptr(str.data()), _len(str.length())
    {
    }
    constexpr string_ref(char_t const *s, size_type count) : _ptr(s), _len(count)
    {
    }
    constexpr string_ref(char_t const *s) : _ptr(s), _len(traits_type::length(s))
    {
    }
    
    string_ref &operator = (string_ref const &) = default;
    
    constexpr const_iterator begin() const
    {
        return _ptr;
    }
    constexpr const_iterator end() const
    {
        return _ptr + _len;
    }
    constexpr const_iterator cbegin() const
    {
        return _ptr;
    }
    constexpr const_iterator cend() const
    {
        return _ptr + _len;
    }
    constexpr const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(_ptr);
    }
    constexpr const_reverse_iterator rend() const
    {
        return const_reverse_iterator(_ptr + _len);
    }
    constexpr const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(_ptr);
    }
    constexpr const_reverse_iterator crend() const
    {
        return const_reverse_iterator(_ptr + _len);
    }
    
    constexpr const_reference operator[](size_type index) const
    {
        return _ptr[index];
    }
    constexpr const_reference at(size_type index) const
    {
        return index >= size() ? throw std::out_of_range("string_ref out of range") : _ptr[index];
    }
    
    constexpr const_reference front() const
    {
        return *_ptr;
    }
    constexpr const_reference back() const
    {
        return *(_ptr + _len - 1);
    }
    
    const_pointer data() const
    {
        return _ptr;
    }
    
    constexpr size_type size() const
    {
        return _len;
    }
    constexpr size_type length() const
    {
        return _len;
    }
    constexpr size_type max_size() const
    {
        return _len;
    }
    
    void clear()
    {
        _len = 0;
    }
    constexpr bool empty() const
    {
        return _len == 0;
    }
    
    void remove_prefix(size_type n)
    {
        n = std::min(n, size());
        _ptr += n;
        _len -= n;
    }
    void remove_suffix(size_type n)
    {
        n = std::min(n, size());
        _len -= n;
    }
    void swap(string_ref &other)
    {
        std::swap(*this, other);
    }
    
    template<class allocator_t = std::allocator<value_type>> std::basic_string<char_t, traits_t, allocator_t> to_string(allocator_t const &a = allocator_t()) const
    {
        return std::basic_string<char_t, traits_t, allocator_t>(data(), length(), a);
    }
    template<class allocator_t> operator std::basic_string<char_t, traits_t, allocator_t>() const
    {
        return std::basic_string<char_t, traits_t, allocator_t>(data(), length());
    }
    template<class to_t> typename std::enable_if<std::is_integral<to_t>::value, to_t>::type to_value() const
    {
        return string_to_integer<value_type, to_t>(_ptr, _len);
    }
    template<class to_t> typename std::enable_if<std::is_floating_point<to_t>::value, to_t>::type to_value() const
    {
        return string_to_real<value_type, to_t>(_ptr, _len);
    }
    
    size_type copy(value_type *dest, size_type count, size_type pos = 0) const
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
    
    constexpr int compare(string_ref v) const
    {
        return compare_helper(traits_t::compare(data(), v.data(), std::min(size(), v.size())), size(), v.size());
    }
    constexpr int compare(size_type pos1, size_type count1, string_ref v) const
    {
        return substr(pos1, count1).compare(v);
    }
    constexpr int compare(size_type pos1, size_type count1, string_ref const &v, size_type pos2, size_type count2) const
    {
        return substr(pos1, count1).compare(v.substr(pos2, count2));
    }
    constexpr int compare(value_type const *s) const
    {
        return compare(string_ref(s));
    }
    constexpr int compare(size_type pos1, size_type count1, value_type const *s) const
    {
        return substr(pos1, count1).compare(string_ref(s));
    }
    constexpr int compare(size_type pos1, size_type count1, value_type const *s, size_type count2) const
    {
        return substr(pos1, count1).compare(string_ref(s, count2));
    }
    
    size_type find(string_ref v, size_type pos = 0) const
    {
        if(pos + v.size() > size() || v.empty())
        {
            return npos;
        }
        for(auto f = std::find(cbegin() + pos, cend(), v.front()); size_type(cend() - f) > v.size(); f = std::find(std::next(f), cend(), v.front()))
        {
            if(string_ref(f, v.size()).compare(v) == 0)
            {
                return f - cbegin();
            }
        }
        return npos;
    }
    size_type find(value_type c, size_type pos = 0) const
    {
        if(pos >= size())
        {
            return npos;
        }
        auto f = std::find(cbegin() + pos, cend(), c);
        return f == cend() ? npos : f - cbegin();
    }
    size_type find(value_type const *s, size_type pos, size_type count) const
    {
        return find(string_ref(s, count), pos);
    }
    size_type find(value_type const *s, size_type pos = 0) const
    {
        return find(string_ref(s), pos);
    }
    
    //constexpr size_type rfind(string_ref const &v, size_type pos = 0) const;
    //constexpr size_type rfind(value_type c, size_type pos = 0) const;
    //constexpr size_type rfind(value_type const *s, size_type pos, size_type count) const;
    //constexpr size_type rfind(value_type const *s, size_type pos = 0) const;
    
    //constexpr size_type find_first_of(string_ref const &v, size_type pos = 0) const;
    //constexpr size_type find_first_of(value_type c, size_type pos = 0) const;
    //constexpr size_type find_first_of(value_type const *s, size_type pos, size_type count) const;
    //constexpr size_type find_first_of(value_type const *s, size_type pos = 0) const;
    
    //constexpr size_type find_last_of(string_ref const &v, size_type pos = 0) const;
    //constexpr size_type find_last_of(value_type c, size_type pos = 0) const;
    //constexpr size_type find_last_of(value_type const *s, size_type pos, size_type count) const;
    //constexpr size_type find_last_of(value_type const *s, size_type pos = 0) const;
    
    //constexpr size_type find_first_not_of(string_ref const &v, size_type pos = 0) const;
    //constexpr size_type find_first_not_of(value_type c, size_type pos = 0) const;
    //constexpr size_type find_first_not_of(value_type const *s, size_type pos, size_type count) const;
    //constexpr size_type find_first_not_of(value_type const *s, size_type pos = 0) const;
    
    //constexpr size_type find_last_not_of(string_ref const &v, size_type pos = 0) const;
    //constexpr size_type find_last_not_of(value_type c, size_type pos = 0) const;
    //constexpr size_type find_last_not_of(value_type const *s, size_type pos, size_type count) const;
    //constexpr size_type find_last_not_of(value_type const *s, size_type pos = 0) const;
    
    static constexpr size_type npos = size_type(-1);
    
private:
    static constexpr int compare_helper(int c, size_type l, size_type r)
    {
        return c != 0 ? c : l == r ? 0 : l < r ? -1 : 1;
    }
    
private:
    value_type const *_ptr;
    size_type _len;
};

template<class char_t, class traits_t> constexpr bool operator == (string_ref<char_t, traits_t> left, string_ref<char_t, traits_t> right){ return left.compare(right) == 0; }
template<class char_t, class traits_t> constexpr bool operator != (string_ref<char_t, traits_t> left, string_ref<char_t, traits_t> right){ return left.compare(right) != 0; }
template<class char_t, class traits_t> constexpr bool operator <  (string_ref<char_t, traits_t> left, string_ref<char_t, traits_t> right){ return left.compare(right) <  0; }
template<class char_t, class traits_t> constexpr bool operator >  (string_ref<char_t, traits_t> left, string_ref<char_t, traits_t> right){ return left.compare(right) >  0; }
template<class char_t, class traits_t> constexpr bool operator <= (string_ref<char_t, traits_t> left, string_ref<char_t, traits_t> right){ return left.compare(right) <= 0; }
template<class char_t, class traits_t> constexpr bool operator >= (string_ref<char_t, traits_t> left, string_ref<char_t, traits_t> right){ return left.compare(right) >= 0; }
template<class char_t, class traits_t> constexpr bool operator == (char_t const *left, string_ref<char_t, traits_t> right){ return right.compare(left) == 0; }
template<class char_t, class traits_t> constexpr bool operator != (char_t const *left, string_ref<char_t, traits_t> right){ return right.compare(left) != 0; }
template<class char_t, class traits_t> constexpr bool operator <  (char_t const *left, string_ref<char_t, traits_t> right){ return right.compare(left) >  0; }
template<class char_t, class traits_t> constexpr bool operator >  (char_t const *left, string_ref<char_t, traits_t> right){ return right.compare(left) <  0; }
template<class char_t, class traits_t> constexpr bool operator <= (char_t const *left, string_ref<char_t, traits_t> right){ return right.compare(left) >= 0; }
template<class char_t, class traits_t> constexpr bool operator >= (char_t const *left, string_ref<char_t, traits_t> right){ return right.compare(left) <= 0; }
template<class char_t, class traits_t> constexpr bool operator == (string_ref<char_t, traits_t> left, char_t const *right){ return left.compare(right) == 0; }
template<class char_t, class traits_t> constexpr bool operator != (string_ref<char_t, traits_t> left, char_t const *right){ return left.compare(right) != 0; }
template<class char_t, class traits_t> constexpr bool operator <  (string_ref<char_t, traits_t> left, char_t const *right){ return left.compare(right) <  0; }
template<class char_t, class traits_t> constexpr bool operator >  (string_ref<char_t, traits_t> left, char_t const *right){ return left.compare(right) >  0; }
template<class char_t, class traits_t> constexpr bool operator <= (string_ref<char_t, traits_t> left, char_t const *right){ return left.compare(right) <= 0; }
template<class char_t, class traits_t> constexpr bool operator >= (string_ref<char_t, traits_t> left, char_t const *right){ return left.compare(right) >= 0; }
template<class char_t, class traits_t, class allocator_t> bool operator == (std::basic_string<char_t, traits_t, allocator_t> const &left, string_ref<char_t, traits_t> right){ return right.compare(string_ref<char_t, traits_t>(left)) == 0; }
template<class char_t, class traits_t, class allocator_t> bool operator != (std::basic_string<char_t, traits_t, allocator_t> const &left, string_ref<char_t, traits_t> right){ return right.compare(string_ref<char_t, traits_t>(left)) != 0; }
template<class char_t, class traits_t, class allocator_t> bool operator <  (std::basic_string<char_t, traits_t, allocator_t> const &left, string_ref<char_t, traits_t> right){ return right.compare(string_ref<char_t, traits_t>(left)) >  0; }
template<class char_t, class traits_t, class allocator_t> bool operator >  (std::basic_string<char_t, traits_t, allocator_t> const &left, string_ref<char_t, traits_t> right){ return right.compare(string_ref<char_t, traits_t>(left)) <  0; }
template<class char_t, class traits_t, class allocator_t> bool operator <= (std::basic_string<char_t, traits_t, allocator_t> const &left, string_ref<char_t, traits_t> right){ return right.compare(string_ref<char_t, traits_t>(left)) >= 0; }
template<class char_t, class traits_t, class allocator_t> bool operator >= (std::basic_string<char_t, traits_t, allocator_t> const &left, string_ref<char_t, traits_t> right){ return right.compare(string_ref<char_t, traits_t>(left)) <= 0; }
template<class char_t, class traits_t, class allocator_t> bool operator == (string_ref<char_t, traits_t> left, std::basic_string<char_t, traits_t, allocator_t> const &right){ return left.compare(string_ref<char_t, traits_t>(right)) == 0; }
template<class char_t, class traits_t, class allocator_t> bool operator != (string_ref<char_t, traits_t> left, std::basic_string<char_t, traits_t, allocator_t> const &right){ return left.compare(string_ref<char_t, traits_t>(right)) != 0; }
template<class char_t, class traits_t, class allocator_t> bool operator <  (string_ref<char_t, traits_t> left, std::basic_string<char_t, traits_t, allocator_t> const &right){ return left.compare(string_ref<char_t, traits_t>(right)) <  0; }
template<class char_t, class traits_t, class allocator_t> bool operator >  (string_ref<char_t, traits_t> left, std::basic_string<char_t, traits_t, allocator_t> const &right){ return left.compare(string_ref<char_t, traits_t>(right)) >  0; }
template<class char_t, class traits_t, class allocator_t> bool operator <= (string_ref<char_t, traits_t> left, std::basic_string<char_t, traits_t, allocator_t> const &right){ return left.compare(string_ref<char_t, traits_t>(right)) <= 0; }
template<class char_t, class traits_t, class allocator_t> bool operator >= (string_ref<char_t, traits_t> left, std::basic_string<char_t, traits_t, allocator_t> const &right){ return left.compare(string_ref<char_t, traits_t>(right)) >= 0; }

template<class char_t, class traits_t = std::char_traits<char_t>, class string_t = string_ref<char_t, traits_t>> struct split_iterator_finder_char
{
public:
    typedef char_t char_type;
    typedef traits_t traits_type;
    typedef string_t string_type;
    
    split_iterator_finder_char() : _find(), _clear()
    {
    }
    split_iterator_finder_char(split_iterator_finder_char const &) = default;
    split_iterator_finder_char(char_t find) : _find(find), _clear()
    {
    }
    typename string_type::size_type run(string_type ref) const
    {
        return ref.find(_find);
    }
    typename string_type::size_type size() const
    {
        return 1;
    }
    bool operator == (split_iterator_finder_char const &other) const
    {
        return _find == other._find && _clear == other._clear;
    }
    bool operator != (split_iterator_finder_char const &other) const
    {
        return _find != other._find || _clear != other._clear;
    }
    void clear()
    {
        _clear = true;
    }
    operator bool() const
    {
        return !_clear;
    }
    bool operator!() const
    {
        return _clear;
    }
private:
    char_t _find;
    bool _clear;
};
template<class char_t, class traits_t = std::char_traits<char_t>, class string_t = string_ref<char_t, traits_t>> struct split_iterator_finder_string
{
public:
    typedef char_t char_type;
    typedef traits_t traits_type;
    typedef string_t string_type;
    
    split_iterator_finder_string() : _find()
    {
    }
    split_iterator_finder_string(split_iterator_finder_string const &) = default;
    split_iterator_finder_string(string_type find) : _find(find)
    {
    }
    typename string_type::size_type run(string_type ref) const
    {
        return ref.find(_find);
    }
    typename string_type::size_type size() const
    {
        return _find.size();
    }
    bool operator == (split_iterator_finder_string const &other) const
    {
        return _find == other._find;
    }
    bool operator != (split_iterator_finder_string const &other) const
    {
        return _find != other._find;
    }
    void clear()
    {
        _find.clear();
    }
    operator bool() const
    {
        return !_find.empty();
    }
    bool operator!() const
    {
        return _find.empty();
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
    
    split_iterator_finder_any_of() : _find()
    {
    }
    split_iterator_finder_any_of(split_iterator_finder_any_of const &) = default;
    split_iterator_finder_any_of(string_type find) : _find(find)
    {
    }
    typename string_type::size_type run(string_type ref) const
    {
        for(typename string_type::size_type pos = 0; pos < ref.size(); ++pos)
        {
            if(std::find(_find.cbegin(), _find.cend(), ref[pos]) != _find.cend())
            {
                return pos;
            }
        }
        return string_type::npos;
    }
    typename string_type::size_type size() const
    {
        return 1;
    }
    bool operator == (split_iterator_finder_any_of const &other) const
    {
        return _find == other._find;
    }
    bool operator != (split_iterator_finder_any_of const &other) const
    {
        return _find != other._find;
    }
    void clear()
    {
        _find.clear();
    }
    operator bool() const
    {
        return !_find.empty();
    }
    bool operator!() const
    {
        return _find.empty();
    }
private:
    string_type _find;
};
template<class finder_t> class split_iterator
{
public:
    typedef std::input_iterator_tag iterator_category;
    typedef typename finder_t::string_type value_type, string_type;
    typedef std::ptrdiff_t difference_type;
    typedef value_type const &reference;
    typedef value_type const *pointer;
    typedef value_type const &const_reference;
    typedef value_type const *const_pointer;
    
    split_iterator(finder_t const &finder) : _current(), _ref(), _finder(finder)
    {
        _finder.clear();
    }
    split_iterator(split_iterator const &) = default;
    split_iterator(split_iterator &&) = default;
    template<class iterator_t> split_iterator(iterator_t begin, iterator_t end, finder_t &&finder) : _current(), _ref(&*begin, end - begin), _finder(std::move(finder))
    {
        _current = _ref.substr(0, _finder.run(_ref));
        _ref.remove_prefix(_current.size());
    }
    split_iterator &operator = (split_iterator const &) = default;
    split_iterator &operator = (split_iterator &&) = default;
    
    split_iterator &operator++()
    {
        if(_ref.empty())
        {
            _finder.clear();
            _current.clear();
        }
        else
        {
            _ref.remove_prefix(_finder.size());
            _current = _ref.substr(0, _finder.run(_ref));
            _ref.remove_prefix(_current.size());
        }
        return *this;
    }
    split_iterator operator++(int)
    {
        split_iterator save(*this);
        ++*this;
        return save;
    }
    const_reference operator *() const
    {
        return _current;
    }
    const_pointer operator->() const
    {
        return &_current;
    }
    bool operator == (split_iterator const &other) const
    {
        if(*this ^ other)
        {
            return false;
        }
        if(!*this && !other)
        {
            return true;
        }
        return _current == other._current && _ref == other._ref && _finder == other._finder;
    }
    bool operator != (split_iterator const &other) const
    {
        return !(*this == other);
    }
    
    split_iterator begin() const
    {
        return *this;
    }
    split_iterator end() const
    {
        return split_iterator(_finder);
    }
    
    operator bool() const
    {
        return _finder;
    }
    bool operator!() const
    {
        return !_finder;
    }
    
    bool eof() const
    {
        return !*this;
    }
    bool next()
    {
        return ++*this;
    }
    string_type other() const
    {
        return _ref.substr(_finder.size());
    }
    
private:
    string_type _current;
    string_type _ref;
    finder_t _finder;
};

template<class iterator_t> split_iterator<split_iterator_finder_char<typename std::iterator_traits<iterator_t>::value_type>> make_split(iterator_t begin, iterator_t end, typename std::iterator_traits<iterator_t>::value_type find)
{
    return split_iterator<split_iterator_finder_char<typename std::iterator_traits<iterator_t>::value_type>>(begin, end, {find});
};
template<class char_t, class traits_t, class allocator_t> split_iterator<split_iterator_finder_char<char_t, traits_t>> make_split(std::basic_string<char_t, traits_t, allocator_t> const &str, char_t find)
{
    return split_iterator<split_iterator_finder_char<char_t, traits_t>>(str.begin(), str.end(), {find});
};
template<class char_t, class traits_t> split_iterator<split_iterator_finder_char<char_t, traits_t>> make_split(string_ref<char_t, traits_t> str, char_t find)
{
    return split_iterator<split_iterator_finder_char<char_t, traits_t>>(str.begin(), str.end(), {find});
};


template<class iterator_t> split_iterator<split_iterator_finder_string<typename std::iterator_traits<iterator_t>::value_type>> make_split(iterator_t begin, iterator_t end, typename std::iterator_traits<iterator_t>::value_type const *find)
{
    return split_iterator<split_iterator_finder_string<typename std::iterator_traits<iterator_t>::value_type>>(begin, end, {find});
};
template<class char_t, class traits_t, class allocator_t> split_iterator<split_iterator_finder_string<char_t, traits_t>> make_split(std::basic_string<char_t, traits_t, allocator_t> const &str, char_t const *find)
{
    return split_iterator<split_iterator_finder_string<char_t, traits_t>>(str.begin(), str.end(), {find});
};
template<class char_t, class traits_t> split_iterator<split_iterator_finder_string<char_t, traits_t>> make_split(string_ref<char_t, traits_t> str, char_t const *find)
{
    return split_iterator<split_iterator_finder_string<char_t, traits_t>>(str.begin(), str.end(), {find});
};

template<class iterator_t, class traits_t, class allocator_t> split_iterator<split_iterator_finder_string<typename std::iterator_traits<iterator_t>::value_type, traits_t>> make_split(iterator_t begin, iterator_t end, std::basic_string<typename std::iterator_traits<iterator_t>::value_type, traits_t, allocator_t> const &find)
{
    return split_iterator<split_iterator_finder_string<typename std::iterator_traits<iterator_t>::value_type>>(begin, end, {find});
};
template<class char_t, class traits_t, class allocator_t> split_iterator<split_iterator_finder_string<char_t, traits_t>> make_split(std::basic_string<char_t, traits_t, allocator_t> const &str, std::basic_string<char_t, traits_t, allocator_t> const &find)
{
    return split_iterator<split_iterator_finder_string<char_t, traits_t>>(str.begin(), str.end(), {find});
};
template<class char_t, class traits_t, class allocator_t> split_iterator<split_iterator_finder_string<char_t, traits_t>> make_split(string_ref<char_t, traits_t> str, std::basic_string<char_t, traits_t, allocator_t> const &find)
{
    return split_iterator<split_iterator_finder_string<char_t, traits_t>>(str.begin(), str.end(), {find});
};

template<class iterator_t> split_iterator<split_iterator_finder_any_of<typename std::iterator_traits<iterator_t>::value_type>> make_split_any_of(iterator_t begin, iterator_t end, typename std::iterator_traits<iterator_t>::value_type const *find)
{
    return split_iterator<split_iterator_finder_any_of<typename std::iterator_traits<iterator_t>::value_type>>(begin, end, {find});
};
template<class char_t, class traits_t, class allocator_t> split_iterator<split_iterator_finder_any_of<char_t, traits_t>> make_split_any_of(std::basic_string<char_t, traits_t, allocator_t> const &str, char_t const *find)
{
    return split_iterator<split_iterator_finder_any_of<char_t, traits_t>>(str.begin(), str.end(), {find});
};
template<class char_t, class traits_t> split_iterator<split_iterator_finder_any_of<char_t, traits_t>> make_split_any_of(string_ref<char_t, traits_t> str, char_t const *find)
{
    return split_iterator<split_iterator_finder_any_of<char_t, traits_t>>(str.begin(), str.end(), {find});
};

template<class iterator_t, class traits_t, class allocator_t> split_iterator<split_iterator_finder_any_of<typename std::iterator_traits<iterator_t>::value_type, traits_t>> make_split_any_of(iterator_t begin, iterator_t end, std::basic_string<typename std::iterator_traits<iterator_t>::value_type, traits_t, allocator_t> const &find)
{
    return split_iterator<split_iterator_finder_any_of<typename std::iterator_traits<iterator_t>::value_type>>(begin, end, {find});
};
template<class char_t, class traits_t, class allocator_t> split_iterator<split_iterator_finder_any_of<char_t, traits_t>> make_split_any_of(std::basic_string<char_t, traits_t, allocator_t> const &str, std::basic_string<char_t, traits_t, allocator_t> const &find)
{
    return split_iterator<split_iterator_finder_any_of<char_t, traits_t>>(str.begin(), str.end(), {find});
};
template<class char_t, class traits_t, class allocator_t> split_iterator<split_iterator_finder_any_of<char_t, traits_t>> make_split_any_of(string_ref<char_t, traits_t> str, std::basic_string<char_t, traits_t, allocator_t> const &find)
{
    return split_iterator<split_iterator_finder_any_of<char_t, traits_t>>(str.begin(), str.end(), {find});
};

