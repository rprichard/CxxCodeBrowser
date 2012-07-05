#include "SourcesJsonReader.h"
#include "Program.h"
#include "Source.h"
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

static Program *readSourcesJson(const Json::Value &json)
{
    Program *program = new Program;
    for (Json::ValueIterator it = json.begin(), itEnd = json.end();
            it != itEnd; ++it) {
        Json::Value &sourceJson = *it;
        Source *source = new Source;
        source->path = QString::fromStdString(sourceJson["file"].asString());
        source->defines = readJsonStringList(sourceJson["defines"]);
        source->includes = readJsonStringList(sourceJson["includes"]);
        source->extraArgs = readJsonStringList(sourceJson["extraArgs"]);
        program->sources.push_back(source);
    }
    return program;
}

Program *readSourcesJson(const QString &filename)
{
    std::ifstream f(filename.toStdString().c_str());
    Json::Reader r;
    Json::Value rootJson;
    r.parse(f, rootJson);
    return readSourcesJson(rootJson);
}

} // namespace Nav
