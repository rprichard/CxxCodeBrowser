#ifndef NAV_MISC_H
#define NAV_MISC_H

class QFontMetrics;

namespace Nav {

template <typename T> void deleteAll(T &t) {
    for (typename T::iterator it = t.begin(); it != t.end(); ++it)
        delete *it;
}

int effectiveLineSpacing(const QFontMetrics &fm);

void hackSwitchIconThemeToTheOneWithIcons();

extern const char placeholderText[];

} // namespace Nav

#endif // NAV_MISC_H
