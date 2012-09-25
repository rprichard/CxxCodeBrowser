#include <iostream>

#include "../libindexdb/IndexDb.h"

int main(int argc, char *argv[])
{
    if (argc != 2 || !strcmp(argv[1], "--help")) {
        std::cout << "Usage:" << argv[0] << " indexdb-file" << std::endl;
        return 1;
    }

    indexdb::Index index(argv[1]);

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
