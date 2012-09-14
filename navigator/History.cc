#include "History.h"

#include <cassert>

namespace Nav {

// Allow at most this many items before and after the current item.
const int kMaximumHistoryLocations = 100;

History::History() : m_historyIndex(-1)
{
}

void History::recordJump(const Location &from, const Location &to)
{
    recordLocation(from);
    recordLocation(to);
}

void History::recordLocation(const Location &loc)
{
    assert(m_historyIndex < m_history.size());

    if (loc.file == NULL)
        return;

    if (m_historyIndex != -1) {
        if (m_history[m_historyIndex] == loc) {
            // Do nothing.
            return;
        } else if (m_historyIndex + 1 < m_history.size() &&
                   m_history[m_historyIndex + 1] == loc) {
            // Don't record this entry -- just advance forward to it.
            m_historyIndex++;
            checkHistoryIndex();
            return;
        }
    }

    // Insert the entry one step forward.
    m_history.insert(m_historyIndex + 1, loc);
    m_historyIndex++;
    checkHistoryIndex();

    capHistorySize();
}

const History::Location *History::goBack()
{
    if (m_historyIndex - 1 < 0) {
        return NULL;
    } else {
        m_historyIndex--;
        capHistorySize();
        return &m_history[m_historyIndex];
    }
}

const History::Location *History::goForward()
{
    if (m_historyIndex + 1 >= m_history.size()) {
        return NULL;
    } else {
        m_historyIndex++;
        capHistorySize();
        return &m_history[m_historyIndex];
    }
}

bool History::canGoBack()
{
    return m_historyIndex >= 1;
}

bool History::canGoForward()
{
    return m_historyIndex + 1 < m_history.size();
}

void History::checkHistoryIndex()
{
    if (m_history.isEmpty()) {
        assert(m_historyIndex == -1);
    } else {
        assert(m_historyIndex >= 0 && m_historyIndex < m_history.size());
    }
}

void History::capHistorySize()
{
    assert(kMaximumHistoryLocations >= 1);
    int toRemove;
    toRemove = m_historyIndex - kMaximumHistoryLocations;
    if (toRemove > 0) {
        m_history.erase(m_history.begin(), m_history.begin() + toRemove);
        m_historyIndex -= toRemove;
    }
    toRemove = m_history.size() - m_historyIndex - 1 - kMaximumHistoryLocations;
    if (toRemove > 0) {
        m_history.erase(m_history.end() - toRemove, m_history.end());
    }
    checkHistoryIndex();
}

} // namespace Nav
