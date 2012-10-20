#ifndef NAV_REGEX_H
#define NAV_REGEX_H

#include <string>
#include <memory>

namespace re2 {
    class RE2;
}

namespace Nav {

class Regex {
public:
    Regex(const std::string &pattern);
    Regex(const Regex &other);
    Regex &operator=(const Regex &other);
    ~Regex();
    bool valid() const;
    re2::RE2 &re2() const                   { return *m_re2; }
    bool match(const char *text) const;
private:
    void initWithPattern(const std::string &pattern);
    std::unique_ptr<re2::RE2> m_re2;
};

bool operator==(const Regex &x, const Regex &y);
inline bool operator!=(const Regex &x, const Regex &y) { return !(x == y); }

} // namespace Nav

#endif // NAV_REGEX_H
