#include <iostream>

#include "../libindexdb/IndexDb.h"

static void dump(const std::string &path)
{
    indexdb::Index index(path);

    printf("String Tables:\n\n");
    printf("    %-20s  %10s  %10s\n", "Name", "Count", "ContentSize");
    printf("    %-20s  %10s  %10s\n", "====", "=====", "===========");
    for (size_t tableIndex = 0; tableIndex < index.stringTableCount();
            ++tableIndex) {
        std::string name = index.stringTableName(tableIndex);
        indexdb::StringTable *table = index.stringTable(name);
        printf("    %-20s  %10u  %10u\n",
               name.c_str(), table->size(), table->contentByteSize());

    }

    printf("\nTables:\n\n");
    printf("    %-20s  %10s  %10s\n", "Name", "Count", "BufferSize");
    printf("    %-20s  %10s  %10s\n", "====", "=====", "==========");
    for (size_t tableIndex = 0; tableIndex < index.tableCount();
            ++tableIndex) {
        std::string name = index.tableName(tableIndex);
        indexdb::Table *table = index.table(name);
        printf("    %-20s  %10d  %10d\n",
               name.c_str(), table->size(), table->bufferSize());
    }
}

static void dumpJson(const std::string &path)
{
    indexdb::Index index(path);

    std::cout << "{" << std::endl;

    std::cout << "\"StringTables\" : {" << std::endl;
    for (size_t tableIndex = 0; tableIndex < index.stringTableCount();
            ++tableIndex) {
        std::string name = index.stringTableName(tableIndex);
        std::cout << "  \"" << name << "\" : [" << std::endl;
        indexdb::StringTable *table = index.stringTable(name);
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
        indexdb::Table *table = index.table(tableName);
        const int columnCount = table->columnCount();
        std::vector<indexdb::StringTable*> columnStringTables(columnCount);
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
                    indexdb::StringTable *stringTable =
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
    if (argc == 3 && !strcmp(argv[1], "--dump")) {
        dump(argv[2]);
    } else if (argc == 3 && !strcmp(argv[1], "--dump-json")) {
        dumpJson(argv[2]);
    } else {
        std::cout << "Usage:" << argv[0] << " (--dump|--dump-json) indexdb-file"
                  << std::endl;
    }
}
