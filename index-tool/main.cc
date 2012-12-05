#include <cstdio>
#include <iostream>

#include "../libindexdb/FileIo.h"
#include "../libindexdb/IndexArchiveReader.h"
#include "../libindexdb/IndexDb.h"

static void dump(const indexdb::Index &index)
{
    printf("String Tables:\n\n");
    printf("    %-20s  %10s  %10s\n", "Name", "Count", "ContentSize");
    printf("    %-20s  %10s  %10s\n", "====", "=====", "===========");
    for (size_t tableIndex = 0; tableIndex < index.stringTableCount();
            ++tableIndex) {
        std::string name = index.stringTableName(tableIndex);
        const indexdb::StringTable *table = index.stringTable(name);
        printf("    %-20s  %10u  %10u\n",
               name.c_str(), table->size(), table->contentByteSize());

    }

    printf("\nTables:\n\n");
    printf("    %-20s  %10s  %10s\n", "Name", "Count", "BufferSize");
    printf("    %-20s  %10s  %10s\n", "====", "=====", "==========");
    for (size_t tableIndex = 0; tableIndex < index.tableCount();
            ++tableIndex) {
        std::string name = index.tableName(tableIndex);
        const indexdb::Table *table = index.table(name);
        printf("    %-20s  %10d  %10d\n",
               name.c_str(), table->size(), table->bufferSize());
    }
}

static void dumpJson(const indexdb::Index &index)
{
    std::cout << "{" << std::endl;

    std::cout << "\"StringTables\" : {" << std::endl;
    for (size_t tableIndex = 0; tableIndex < index.stringTableCount();
            ++tableIndex) {
        std::string name = index.stringTableName(tableIndex);
        std::cout << "  \"" << name << "\" : [" << std::endl;
        const indexdb::StringTable *table = index.stringTable(name);
        for (size_t stringIndex = 0, stringCount = table->size();
                stringIndex < stringCount; ++stringIndex) {
            std::cout << "    \"" << table->item(stringIndex) << "\"";
            if (stringIndex + 1 < stringCount)
                std::cout << ',';
            std::cout << std::endl;
        }
        std::cout << "  ]";
        if (tableIndex + 1 < index.stringTableCount())
            std::cout << ',';
        std::cout << std::endl;
    }
    std::cout << "}," << std::endl;

    std::cout << "\"Tables\" : {" << std::endl;
    for (size_t tableIndex = 0; tableIndex < index.tableCount();
            ++tableIndex) {
        const std::string tableName = index.tableName(tableIndex);
        std::cout << "  \"" << tableName << "\" : {" << std::endl;

        std::cout << "    \"ColumnStringTables\" : [" << std::endl;
        const indexdb::Table *table = index.table(tableName);
        const int columnCount = table->columnCount();
        std::vector<const indexdb::StringTable*> columnStringTables(columnCount);
        for (int columnIndex = 0; columnIndex < columnCount; ++columnIndex) {
            std::string columnName = table->columnName(columnIndex);
            std::cout << "      \"" << columnName << '"';
            if (columnIndex + 1 < columnCount)
                std::cout << ',';
            std::cout << std::endl;
            if (!columnName.empty()) {
                columnStringTables[columnIndex] =
                        index.stringTable(columnName);
            }
        }
        std::cout << "    ]," << std::endl;

        {
            std::cout << "    \"Rows\" : [" << std::endl;
            indexdb::Row tableRow(table->columnCount());
            indexdb::Table::iterator it = table->begin();
            indexdb::Table::iterator itEnd = table->end();
            while (it != itEnd) {
                it.value(tableRow);
                std::cout << "      [";
                for (int columnIndex = 0; columnIndex < columnCount;
                        ++columnIndex) {
                    const indexdb::StringTable *stringTable =
                            columnStringTables[columnIndex];
                    if (stringTable != NULL) {
                        std::cout << '"'
                                  << stringTable->item(tableRow[columnIndex])
                                  << '"';
                    } else {
                        std::cout << tableRow[columnIndex];
                    }
                    if (columnIndex + 1 < columnCount)
                        std::cout << ", ";
                }
                std::cout << ']';
                ++it;
                if (it != itEnd)
                    std::cout << ',';
                std::cout << std::endl;
            }
        }

        std::cout << "    ]" << std::endl;
        std::cout << "  }";
        if (tableIndex + 1 < index.tableCount())
            std::cout << ',';
        std::cout << std::endl;
    }
    std::cout << '}' << std::endl;
    std::cout << '}' << std::endl;
}

int main(int argc, char *argv[])
{
    // TODO: Improve argument parsing.
    // TODO: Refactor this pile of code.

    if (argc == 3 && !strcmp(argv[1], "--dump")) {
        std::string path = argv[2];
        indexdb::UnmappedReader reader(path);
        if (reader.peekSignature(indexdb::kIndexSignature)) {
            indexdb::Index index(path);
            dump(index);
        } else if (reader.peekSignature(indexdb::kIndexArchiveSignature)) {

            // This code was copied-and-pasted to below.

            indexdb::IndexArchiveReader archive(path);
            for (int i = 0; i < archive.size(); ++i) {
                std::cout << "FILE: " << archive.entry(i).name;
                std::string hash = archive.entry(i).hash;
                if (!hash.empty()) {
                    std::cout << " (hash: ";
                    for (size_t j = 0; j < hash.size(); ++j) {
                        char tmp[30];
                        sprintf(tmp, "%02x", (unsigned char)hash[j]);
                        std::cout << tmp;
                    }
                    std::cout << ")";
                }
                std::cout << std::endl;
                std::cout << std::endl;
                indexdb::Index *index = archive.openEntry(i);
                dump(*index);
                std::cout << std::endl;
                delete index;
            }
        } else {
            std::cerr << "error: " << path << " is not an index file."
                      << std::endl;
        }
    } else if (argc == 3 && !strcmp(argv[1], "--dump-json")) {
        std::string path = argv[2];
        indexdb::UnmappedReader reader(path);
        if (reader.peekSignature(indexdb::kIndexSignature)) {
            indexdb::Index index(path);
            dumpJson(index);
        } else if (reader.peekSignature(indexdb::kIndexArchiveSignature)) {

            // This code was copied-and-pasted from above.
            // TODO: It's not even trying to be a JSON file.  Fix it.

            indexdb::IndexArchiveReader archive(path);
            for (int i = 0; i < archive.size(); ++i) {
                std::cout << "FILE: " << archive.entry(i).name << std::endl;
                indexdb::Index *index = archive.openEntry(i);
                dumpJson(*index);
                delete index;
            }
        } else {
            std::cerr << "error: " << path << " is not an index file."
                      << std::endl;
        }
    } else {
        std::cout << "Usage: " << argv[0] << " (--dump|--dump-json) indexdb-file"
                  << std::endl;
    }
}
