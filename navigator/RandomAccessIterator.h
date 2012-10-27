#ifndef NAV_RANDOMACCESSITERATOR_H
#define NAV_RANDOMACCESSITERATOR_H

#include <iterator>

namespace Nav {

// A general-purpose random-access iterator class.  An iterator contains a
// pointer to its container and an integral index.  When the iterator is
// dereferenced, it calls operator[] on the container.
template <typename Container, typename Value, typename Index>
class RandomAccessIterator {
public:
    RandomAccessIterator(Container &container, Index i) :
        m_container(&container), m_index(i)
    {
    }

    typedef std::random_access_iterator_tag iterator_category;
    typedef Value                           value_type;
    typedef Index                           difference_type;
    typedef Value                           *pointer;
    typedef Value                           &reference;

    const Value &operator*() const { return (*m_container)[m_index]; }
    const Value *operator->() const { return &(*m_container)[m_index]; }
    bool operator==(const RandomAccessIterator &other) const { return m_index == other.m_index; }
    bool operator!=(const RandomAccessIterator &other) const { return m_index != other.m_index; }
    bool operator<(const RandomAccessIterator &other) const { return m_index < other.m_index; }
    bool operator>(const RandomAccessIterator &other) const { return m_index > other.m_index; }
    bool operator<=(const RandomAccessIterator &other) const { return m_index <= other.m_index; }
    bool operator>=(const RandomAccessIterator &other) const { return m_index >= other.m_index; }
    RandomAccessIterator &operator++() { m_index++; return *this; }
    RandomAccessIterator operator++(int) { RandomAccessIterator ret = *this; m_index++; return ret; }
    RandomAccessIterator &operator--() { m_index--; return *this; }
    RandomAccessIterator operator--(int) { RandomAccessIterator ret = *this; m_index--; return ret; }
    difference_type operator-(const RandomAccessIterator &other) const { return m_index - other.m_index; }
    RandomAccessIterator operator+(difference_type i) const { return RandomAccessIterator(*m_container, m_index + i); }
    RandomAccessIterator operator-(difference_type i) const { return RandomAccessIterator(*m_container, m_index - i); }
    RandomAccessIterator &operator+=(difference_type i) { m_index += i; return *this; }
    RandomAccessIterator &operator-=(difference_type i) { m_index -= i; return *this; }

private:
    Container *m_container;
    Index m_index;
};

template <typename Container, typename Value, typename Index>
inline RandomAccessIterator<Container, Value, Index> operator+(
    typename RandomAccessIterator<Container, Value, Index>::difference_type i,
    const RandomAccessIterator<Container, Value, Index> &other)
{
    return other + i;
}

} // namespace Nav

#endif // NAV_RANDOMACCESSITERATOR_H
