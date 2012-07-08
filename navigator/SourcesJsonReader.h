#ifndef NAV_SOURCESJSONREADER_H
#define NAV_SOURCESJSONREADER_H

#include <json/value.h>
#include <QString>

namespace Nav {

class Project;

Project *readSourcesJson(const QString &filename);

} // namespace Nav

#endif // NAV_SOURCESJSONREADER_H
