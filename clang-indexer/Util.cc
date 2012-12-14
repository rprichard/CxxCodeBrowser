#include "Util.h"
#include "../shared_headers/host.h"

#include <climits>
#include <cstdlib>
#include <cstring>
#include <iostream>

#if defined(SOURCEWEB_UNIX)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

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

char *portableRealPath(const char *path)
{
#if defined(SOURCEWEB_UNIX)
    return realpath(path, NULL);
#elif defined(_WIN32)
    return _fullpath(NULL, path, 0);
#else
#error "portableRealPath not implemented for this target."
#endif
}

time_t getPathModTime(const std::string &path)
{
#if defined(SOURCEWEB_UNIX)
    // TODO: What about symlinks?  It seems that the perfect behavior is to use
    // the non-symlink modtime for the index file itself, but for input files,
    // to use the highest modtime among all the symlinks and the non-symlink.
    // That's complicated, though, so just use the modtime of the non-symlink.
    struct stat buf;
    if (stat(path.c_str(), &buf) != 0)
        return kInvalidTime;
    return buf.st_mtime;
#elif defined(_WIN32)
    // [rprichard] 2012-12-04.  Use GetFileAttributesEx instead of stat.  In my
    // experience, stat is unreliable.  It calls FindFirstFile, which has this
    // comment about it on MSDN:
    //    Note:  In rare cases or on a heavily loaded system, file attribute
    //    information on NTFS file systems may not be current at the time this
    //    function is called. To be assured of getting the current NTFS file
    //    system file attributes, call the GetFileInformationByHandle function.
    // I do not want to open the file, so I'm calling GetFileAttributesEx
    // instead.  It would be nice if someone else could corroborate my
    // experience.
    WIN32_FILE_ATTRIBUTE_DATA attrData;
    memset(&attrData, 0, sizeof(attrData));
    if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &attrData))
        return kInvalidTime;
    uint64_t t;
    t = static_cast<uint64_t>(attrData.ftLastWriteTime.dwHighDateTime) << 32;
    t |= attrData.ftLastWriteTime.dwLowDateTime;
    t -= 116444736000000000;
    t /= 10000000;
    return static_cast<time_t>(t);
#else
#error "Not implemented on this OS."
#endif
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
