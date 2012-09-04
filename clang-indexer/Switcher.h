#ifndef INDEXER_SWITCHER_H
#define INDEXER_SWITCHER_H

namespace indexer {

template <typename T>
class Switcher
{
public:
    Switcher(T &var) : m_var(&var), m_origVal(var) {}
    Switcher(T &var, T tempVal) : m_var(&var), m_origVal(var) {
        var = tempVal;
    }
    ~Switcher() {
        *m_var = m_origVal;
    }
private:
    T *m_var;
    T m_origVal;
};

} // namespace indexer

#endif // INDEXER_SWITCHER_H
