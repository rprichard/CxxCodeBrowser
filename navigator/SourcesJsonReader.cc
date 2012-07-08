#include "SourcesJsonReader.h"
#include "Project.h"
#include "CSource.h"
#include <json/reader.h>
#include <string>
#include <fstream>
#include <vector>

namespace Nav {

static QStringList readJsonStringList(const Json::Value &json)
{
    QStringList result;
    for (Json::ValueIterator it = json.begin(), itEnd = json.end();
            it != itEnd; ++it) {
        Json::Value &element = *it;
        result << QString::fromStdString(element.asString());
    }
    return result;
}

static Project *readSourcesJson(const Json::Value &json)
{
    Project *program = new Project;
    for (Json::ValueIterator it = json.begin(), itEnd = json.end();
            it != itEnd; ++it) {
        Json::Value &sourceJson = *it;
        CSource *source = new CSource;
        source->path = QString::fromStdString(sourceJson["file"].asString());
        source->defines = readJsonStringList(sourceJson["defines"]);
        source->includes = readJsonStringList(sourceJson["includes"]);
        source->extraArgs = readJsonStringList(sourceJson["extraArgs"]);
        program->csources << source;
    }
    return program;
}

Project *readSourcesJson(const QString &filename)
{
    std::ifstream f(filename.toStdString().c_str());
    Json::Reader r;
    Json::Value rootJson;
    r.parse(f, rootJson);
    return readSourcesJson(rootJson);
}

} // namespace Nav
