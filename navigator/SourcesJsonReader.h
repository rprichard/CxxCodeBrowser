#ifndef NAV_SOURCESJSONREADER_H
#define NAV_SOURCESJSONREADER_H

#include <json/value.h>
#include <QString>

namespace Nav {

class Program;

Program *readSourcesJson(const QString &filename);

} // namespace Nav

#endif // NAV_SOURCESJSONREADER_H
