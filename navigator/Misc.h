#ifndef NAV_MISC_H
#define NAV_MISC_H

namespace Nav {

template <typename T> void deleteAll(T &t) {
    for (typename T::iterator it = t.begin(); it != t.end(); ++it)
        delete *it;
}

} /* Nav namespace */

#endif // NAV_MISC_H
