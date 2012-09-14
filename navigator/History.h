#ifndef NAV_HISTORY_H
#define NAV_HISTORY_H

#include <QList>
#include <QPoint>

namespace Nav {

class File;

class History
{
public:
    struct Location {
        Location() : file(NULL) {}
        Location(File *file, QPoint offset) : file(file), offset(offset) {}

        File *file;
        QPoint offset;
    };

    History();

    // The history client should call this to record "jumps" to a location.
    void recordJump(const Location &from, const Location &to);

    // The history client should record its current location before invoking
    // back or forward.
    void recordLocation(const Location &loc);

    // Attempt to go back or forward.  Returns NULL on failure.  The "jump"
    // to the location returned by these methods must not be recorded via
    // recordJump.
    const Location *goBack();
    const Location *goForward();

    bool canGoBack();
    bool canGoForward();

private:
    void checkHistoryIndex();
    void capHistorySize();

private:
    QList<Location> m_history;
    int m_historyIndex;
};

inline bool operator==(const History::Location &x, const History::Location &y)
{
    return x.file == y.file && x.offset == y.offset;
}

inline bool operator!=(const History::Location &x, const History::Location &y)
{
    return !(x == y);
}

} // namespace Nav

#endif // NAV_HISTORY_H
