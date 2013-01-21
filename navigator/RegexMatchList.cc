#include "RegexMatchList.h"

#include <cassert>
#include <utility>
#include <re2/prog.h>
#include <re2/re2.h>
#include <re2/regexp.h>
#include <re2/stringpiece.h>

namespace Nav {

RegexMatchList::RegexMatchList() : m_contentString(&m_emptyString)
{
}

RegexMatchList::RegexMatchList(RegexMatchList &&other)
{
    *this = std::move(other);
}

RegexMatchList &RegexMatchList::operator=(RegexMatchList &&other)
{
    assert(this != &other);
    m_contentString = other.m_contentString == &other.m_emptyString
            ? &m_emptyString
            : other.m_contentString;
    m_regex = std::move(other.m_regex);
    m_matchInitFlags = std::move(other.m_matchInitFlags);
    m_matchRanges = std::move(other.m_matchRanges);
    m_reverseProg = std::move(other.m_reverseProg);
    return *this;
}

RegexMatchList::RegexMatchList(
        const std::string &contentString,
        const Regex &regex) :
    m_contentString(&contentString),
    m_regex(regex)
{
    if (m_regex.empty())
        return;
    re2::Regexp &regexp = *m_regex.re2().Regexp();
    std::unique_ptr<re2::Prog> prog =
            std::unique_ptr<re2::Prog>(regexp.CompileToProg(0));
    m_reverseProg =
            std::unique_ptr<re2::Prog>(regexp.CompileToReverseProg(0));
    if (!prog || !m_reverseProg) {
        // TODO: report error?
        return;
    }
    re2::StringPiece content(*m_contentString);
    re2::StringPiece match;
    bool failed;
    int pos = 0;
    int lastMatchEnd = -1;
    while (pos <= content.size()) {
        re2::StringPiece piece(content.data() + pos,
                               content.size() - pos);
        if (!prog->SearchDFA(
                    piece,
                    content,
                    re2::Prog::kUnanchored,
                    re2::Prog::kLongestMatch,
                    &match,
                    &failed,
                    NULL)) {
            break;
        }
        pos = match.data() - content.data();

        // SearchDFA finds the end of the match, but not the beginning.  Record
        // the beginning of the searched chunk -- the actual beginning will be
        // computed lazily to speed up the initial search.
        //
        // Because we do not know where the beginning is, we do not know
        // whether this match's actual length is zero or non-zero.  To ensure
        // that we don't add the same actual match twice by accident, exclude an
        // (empty) match that has the same ending position as the previous
        // match.
        const int matchEnd = pos + match.length();
        if (matchEnd > lastMatchEnd) {
            m_matchInitFlags.push_back(false);
            m_matchRanges.push_back(std::make_pair(pos, matchEnd));
            lastMatchEnd = matchEnd;
        }

        pos = std::max(matchEnd, pos + 1);
    }
}

// Explicitly declare out-of-line destructor to free re2::Prog.
RegexMatchList::~RegexMatchList()
{
}

const RegexMatchList::value_type &RegexMatchList::operator[](int i) const
{
    assert(i < static_cast<int>(m_matchInitFlags.size()));
    auto &ret = m_matchRanges[i];
    if (!m_matchInitFlags[i]) {
        // FindMatchList() recorded the end location of the match, but not
        // an accurate end -- find the beginning here by searching backwards
        // for the longest match.
        int start = ret.first;
        int end = ret.second;
        bool failed;
        re2::StringPiece matchPiece;
        re2::StringPiece piece(m_contentString->data() + start, end - start);
        if (m_reverseProg->SearchDFA(
                    piece,
                    *m_contentString,
                    re2::Prog::kAnchored,
                    re2::Prog::kLongestMatch,
                    &matchPiece,
                    &failed,
                    NULL) && !failed) {
            assert(matchPiece.data() >= piece.data());
            start = matchPiece.data() - m_contentString->data();
        } else {
            // TODO: Why would this fail?
            start = end - 1;
        }
        m_matchInitFlags[i] = true;
        ret.first = start;
    }
    return ret;
}

} // namespace Nav
