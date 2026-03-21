#ifndef COMPILER_H
#define COMPILER_H

#include <vector>
#include <string>
#include <map>
#include <utility> // Tambahkan ini untuk std::pair

enum PSXType { TYPE_TEXT, TYPE_NUMBER, TYPE_DECIMAL, TYPE_BOOL, TYPE_CHAR, TYPE_COMPLEX, TYPE_NULL, TYPE_UNKNOWN };

struct Variable {
    PSXType type;
    std::string value;
};

std::string trim(const std::string& str);
PSXType detectType(std::string val);

// PERBAIKAN: Gunakan std::pair<int, string> agar compiler tahu nomor barisnya
void buildNativeBinary(std::string filename, const std::vector<std::pair<int, std::string>>& codeLines, bool includeEasterEgg);

#endif