#include "TableReport.h"

#include <cstring>
#include <string>

#include "Regex.h"

namespace Nav {

int TableReport::compare(int row1, int row2, int col)
{
    std::string temp1;
    std::string temp2;
    const char *str1 = text(row1, col, temp1);
    const char *str2 = text(row2, col, temp2);
    return strcmp(str1, str2);
}

bool TableReport::filter(int row, const Regex &regex, std::string &tempBuf)
{
    return regex.match(text(row, 0, tempBuf));
}

} // namespace Nav
