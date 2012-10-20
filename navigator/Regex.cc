#include "Regex.h"

#include <re2/re2.h>

using re2::RE2;

namespace Nav {

Regex::Regex(const std::string &pattern)
{
    initWithPattern(pattern);
}

Regex::Regex(const Regex &other)
{
    initWithPattern(other.re2().pattern());
}

Regex &Regex::operator=(const Regex &other)
{
    initWithPattern(other.re2().pattern());
    return *this;
}

void Regex::initWithPattern(const std::string &pattern)
{
    bool caseSensitive = false;
    for (unsigned char ch : pattern) {
        if (isupper(ch)) {
            caseSensitive = true;
            break;
        }
    }
    RE2::Options options;
    options.set_case_sensitive(caseSensitive);
    options.set_log_errors(false);
    m_re2 = std::unique_ptr<RE2>(new RE2(pattern, options));
}

// ~Regex needs to be defined here in Regex.cc because Regex.h leaves re2::RE2
// an incomplete type (i.e. it does not include re2/re2.h).
Regex::~Regex()
{
}

bool Regex::valid() const
{
    return m_re2->ok();
}

bool Regex::match(const char *text) const
{
    return m_re2->Match(text, 0, strlen(text), RE2::UNANCHORED, NULL, 0);
}

bool operator==(const Regex &x, const Regex &y)
{
    return x.re2().pattern() == y.re2().pattern();
}

} // namespace Nav
