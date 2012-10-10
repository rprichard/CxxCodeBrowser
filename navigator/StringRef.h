#ifndef NAV_STRINGREF_H
#define NAV_STRINGREF_H

namespace Nav {

class StringRef {
public:
    StringRef() : m_str(NULL), m_size(0) {}
    StringRef(const char *str, size_t len) : m_str(str), m_size(len) {}
    const char &operator[](size_t index) const { return m_str[index]; }
    size_t size() const { return m_size; }

    // Data is not guaranteed to be NUL-terminated.
    const char *data() { return m_str; }

private:
    const char *m_str;
    size_t m_size;
};

} // namespace Nav

#endif // NAV_STRINGREF_H
