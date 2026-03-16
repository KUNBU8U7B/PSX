#ifndef COMPILER_H
#define COMPILER_H

#include <vector>
#include <string>
#include <map>

enum PSXType { TYPE_TEXT, TYPE_NUMBER, TYPE_DECIMAL, TYPE_BOOL, TYPE_CHAR, TYPE_UNKNOWN };

struct Variable {
    PSXType type;
    std::string value;
};

std::string trim(const std::string& str);
PSXType detectType(std::string val);
void buildNativeBinary(std::string filename, const std::vector<std::string>& codeLines, bool includeEasterEgg);

#endif