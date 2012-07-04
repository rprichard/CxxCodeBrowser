#ifndef NAV_SOURCESJSONREADER_H
#define NAV_SOURCESJSONREADER_H

#include <json/value.h>

namespace Nav {

class Program;

Program *readSourcesJson(const Json::Value &json);
Program *readSourcesJson(const char *filename);

} // namespace Nav

#endif // NAV_SOURCESJSONREADER_H
