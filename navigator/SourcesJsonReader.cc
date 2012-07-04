#include "SourcesJsonReader.h"
#include "Program.h"
#include "Source.h"
#include <json/reader.h>
#include <string>
#include <fstream>
#include <vector>

namespace Nav {

static std::vector<std::string> readJsonStringList(const Json::Value &json)
{
    std::vector<std::string> result;
    for (Json::ValueIterator it = json.begin(), itEnd = json.end();
            it != itEnd; ++it) {
        Json::Value &element = *it;
        result.push_back(element.asString());
    }
    return result;
}

Program *readSourcesJson(const Json::Value &json)
{
    Program *program = new Program;
    for (Json::ValueIterator it = json.begin(), itEnd = json.end();
            it != itEnd; ++it) {
        Json::Value &sourceJson = *it;
        Source *source = new Source;
        source->path = sourceJson["file"].asString();
        source->defines = readJsonStringList(sourceJson["defines"]);
        source->includes = readJsonStringList(sourceJson["includes"]);
        source->extraArgs = readJsonStringList(sourceJson["extraArgs"]);
        program->sources.push_back(source);
    }
    return program;
}

Program *readSourcesJson(const char *filename)
{
    std::ifstream f(filename);
    Json::Reader r;
    Json::Value rootJson;
    r.parse(f, rootJson);
    return readSourcesJson(rootJson);
}

} // namespace Nav
