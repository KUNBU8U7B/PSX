#ifndef COMPILER_H
#define COMPILER_H

#include <vector>
#include <string>
#include <map>

// Definisi Tipe Data PSX yang didukung
enum PSXType { TYPE_TEXT, TYPE_NUMBER, TYPE_DECIMAL, TYPE_BOOL, TYPE_CHAR, TYPE_COMPLEX, TYPE_NULL, TYPE_UNKNOWN };

struct Variable {
    PSXType type;
    std::string value;
};

// Deklarasi fungsi utilitas agar bisa dipakai di main.cpp dan compiler.cpp
std::string trim(const std::string& str);
PSXType detectType(std::string val);
void buildNativeBinary(std::string filename, const std::vector<std::string>& codeLines, bool includeEasterEgg);

#endif