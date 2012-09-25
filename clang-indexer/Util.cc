#include "Util.h"

#include <iostream>

namespace indexer {

// The POSIX basename can modify its path, which is inconvenient.
const char *const_basename(const char *path)
{
    const char *ret = path;
    while (true) {
        char ch = *path;
        if (ch == '\0') {
            break;
        } else if (ch == '/') {
            ret = path + 1;
        }
#ifdef _WIN32
        // Not tested
        else if (ch == '\\' || ch == ':') {
            ret = path + 1;
        }
#endif
        path++;
    }
    return ret;
}

bool stringStartsWith(const std::string &str, const std::string &suffix)
{
    if (suffix.size() > str.size())
        return false;
    return !str.compare(0, suffix.size(), suffix);
}

bool stringEndsWith(const std::string &str, const std::string &suffix)
{
    if (suffix.size() > str.size())
        return false;
    return !str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

std::string readLine(FILE *fp, bool *isEof)
{
    std::string result;
    if (isEof != NULL)
        *isEof = false;
    while (true) {
        int ch = fgetc(fp);
        if (ch == EOF && result.empty() && isEof != NULL)
            *isEof = true;
        if (ch == EOF || ch == '\n' || ch == '\r') {
            if (ch == '\r') {
                int ch2 = fgetc(fp);
                if (ch2 != '\n' && ch2 != EOF)
                    ungetc(ch2, fp);
            }
            break;
        }
        result.push_back(ch);
    }
    return result;
}

} // namespace indexer
