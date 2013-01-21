#ifndef NAV_REGEXMATCHLIST_H
#define NAV_REGEXMATCHLIST_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "RandomAccessIterator.h"
#include "Regex.h"

namespace re2 {
    class Prog;
}

namespace Nav {

// This class is almost redundant with std::vector<std::pair<int, int>>.  The
// major difference is that the start indices in this class are computed
// lazily.
class RegexMatchList {
public:
    // A pair of start and end locations (indices) of a match.
    typedef std::pair<int, int> value_type;
    typedef RandomAccessIterator<const RegexMatchList, const value_type, int> iterator;

    RegexMatchList();
    RegexMatchList(RegexMatchList &&other);
    RegexMatchList &operator=(RegexMatchList &&other);
    ~RegexMatchList();

    // Construct a RegexMatchList containing all instances of the regex in the
    // content string.  The RegexMatchList object keeps a pointer to the
    // content string, but makes a copy of the Regex object.
    RegexMatchList(const std::string &contentString, const Regex &regex);

    // Return the number of matches.
    int size() const { return m_matchInitFlags.size(); }
    bool empty() const { return m_matchInitFlags.empty(); }

    // Return the start and end position of match i.
    const value_type &operator[](int i) const;

    iterator begin() const { return iterator(*this, 0); }
    iterator end() const { return iterator(*this, size()); }

private:
    const std::string m_emptyString;
    const std::string *m_contentString;
    Regex m_regex;
    mutable std::vector<uint8_t> m_matchInitFlags;
    mutable std::vector<value_type> m_matchRanges;
    std::unique_ptr<re2::Prog> m_reverseProg;
};

} // namespace Nav

#endif // NAV_REGEXMATCHLIST_H
