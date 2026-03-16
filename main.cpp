#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "compiler.h"

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

PSXType detectType(std::string val) {
    if (val.empty()) return TYPE_UNKNOWN;
    if (val.front() == '"' && val.back() == '"') return TYPE_TEXT;
    if (val.front() == '\'' && val.back() == '\'' && val.length() >= 3) return TYPE_CHAR;
    if (val == "true" || val == "false") return TYPE_BOOL;
    if (val.find('.') != std::string::npos) return TYPE_DECIMAL;
    return TYPE_NUMBER;
}

class Interpreter {
private:
    std::map<std::string, Variable> symbolTable; // Penyimpanan variabel

    // Fungsi untuk membagi print berdasarkan koma, tapi mengabaikan koma di dalam kutipan
    std::vector<std::string> splitSegments(std::string content) {
        std::vector<std::string> segments;
        std::string current;
        bool inQuotes = false;
        int inParens = 0; // LOGIKA BARU: Penghitung tanda kurung

        for (char c : content) {
            if (c == '"') inQuotes = !inQuotes;
            else if (c == '(' && !inQuotes) inParens++;
            else if (c == ')' && !inQuotes) inParens--;

            // Potong berdasarkan koma HANYA JIKA tidak di dalam kutipan DAN tidak di dalam kurung
            if (c == ',' && !inQuotes && inParens == 0) {
                segments.push_back(trim(current));
                current = "";
            } else {
                current += c;
            }
        }
        segments.push_back(trim(current));
        return segments;
    }

public:
    void execute(std::string line) {
        line = trim(line);
        if (line.empty() || line == "use psx") return;

        // 1. Deklarasi Eksplisit (contoh: number:: x, y)
        if (line.find("::") != std::string::npos) {
            size_t pos = line.find("::");
            std::string typeStr = trim(line.substr(0, pos));
            std::string vars = line.substr(pos + 2);
            
            PSXType type = (typeStr == "text") ? TYPE_TEXT : 
                           (typeStr == "number") ? TYPE_NUMBER : 
                           (typeStr == "decimal") ? TYPE_DECIMAL :
                           (typeStr == "bool") ? TYPE_BOOL : TYPE_CHAR;

            std::stringstream ss(vars);
            std::string v;
            while (std::getline(ss, v, ',')) symbolTable[trim(v)] = {type, ""};
        }
        // 2. Deklarasi Python-style (contoh: x = 10)
        else if (line.find("=") != std::string::npos && (line.length() < 5 || line.substr(0, 5) != "print")) {
            size_t pos = line.find("=");
            std::string varName = trim(line.substr(0, pos));
            std::string val = trim(line.substr(pos + 1));
            
            PSXType type = detectType(val); // Gunakan fungsi deteksi pintar
            symbolTable[varName] = {type, val};
        }
        // 3. Print Gabungan
        else if (line.substr(0, 5) == "print") {
            std::string content = trim(line.substr(6));
            std::vector<std::string> segments = splitSegments(content);

            for (std::string& seg : segments) {
                if (seg.empty()) continue;

                // 1. Cek syntax Label(tipe, variabel)
                if (seg.length() > 7 && seg.substr(0, 6) == "Label(" && seg.back() == ')') {
                    size_t commaPos = seg.find(',');
                    if (commaPos != std::string::npos) {
                        std::string targetType = trim(seg.substr(6, commaPos - 6));
                        std::string varName = trim(seg.substr(commaPos + 1, seg.length() - commaPos - 2));

                        if (symbolTable.count(varName)) {
                            Variable var = symbolTable[varName];
                            bool match = false;
                            if (targetType == "text" && var.type == TYPE_TEXT) match = true;
                            else if (targetType == "number" && var.type == TYPE_NUMBER) match = true;
                            else if (targetType == "decimal" && var.type == TYPE_DECIMAL) match = true;
                            else if (targetType == "bool" && var.type == TYPE_BOOL) match = true;
                            else if (targetType == "char" && var.type == TYPE_CHAR) match = true;

                            if (match) {
                                std::cout << "[" << targetType << ": " << (var.value.front() == '"' ? var.value.substr(1, var.value.length()-2) : var.value) << "]";
                            } else {
                                std::cout << "[TypeError: '" << varName << "' bukan " << targetType << "]";
                            }
                        } else {
                            std::cout << "[Error: Variabel '" << varName << "' tidak ditemukan]";
                        }
                    }
                }
                // 2. Cek jika Literal String "..."
                else if (seg.front() == '"' && seg.back() == '"') {
                    std::cout << seg.substr(1, seg.length() - 2);
                }
                // 3. Cek jika itu Variabel biasa (tanpa label)
                else if (symbolTable.count(seg)) {
                    std::string val = symbolTable[seg].value;
                    if (!val.empty() && val.front() == '"') std::cout << val.substr(1, val.length() - 2);
                    else std::cout << val;
                }
                // 4. Teks/angka biasa
                else {
                    std::cout << seg;
                }
                std::cout << " ";
            }
            std::cout << std::endl;
        }
    }

    void loadFile(std::string filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return;

        std::vector<std::string> allLines;
        std::string line;
        bool hasUsePsx = false;
        bool compileRequested = false;

        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;
            
            if (line == "use psx") {
                hasUsePsx = true;
            }
            if (line == "!compile") {
                compileRequested = true;
                continue;
            }
            allLines.push_back(line);
        }
        file.close();

        // LOGIKA BARU: Cek apakah ini file .psx
        bool isPsxFile = (filename.substr(filename.find_last_of(".") + 1) == "psx");

        // Jika bukan file .psx DAN tidak ada 'use psx', maka tolak
        if (!isPsxFile && !hasUsePsx) {
            std::cerr << "❌ ERROR: File non-.psx wajib menggunakan 'use psx'!" << std::endl;
            return;
        }

        if (compileRequested) {
            buildNativeBinary(filename, allLines, hasUsePsx);
        } else {
            for (const std::string& l : allLines) execute(l);
        }
    }
};

int main(int argc, char* argv[]) {
    Interpreter psx;
    if (argc > 1) {
        psx.loadFile(argv[1]);
    } else {
        std::cout << "PSX Mode Terhubung." << std::endl;
    }
    return 0;
}