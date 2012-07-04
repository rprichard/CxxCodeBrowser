#ifndef NAV_SOURCE_H
#define NAV_SOURCE_H

#include <string>
#include <vector>

namespace Nav {

class Source
{
public:
    std::string path;
    std::vector<std::string> includes;
    std::vector<std::string> defines;
    std::vector<std::string> extraArgs;
};

} // namespace Nav

#endif // NAV_SOURCE_H
