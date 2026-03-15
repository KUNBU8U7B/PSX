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

class Interpreter {
private:
    std::map<std::string, Variable> symbolTable; // Penyimpanan variabel

    // Fungsi untuk membagi print berdasarkan koma, tapi mengabaikan koma di dalam kutipan
    std::vector<std::string> splitSegments(std::string content) {
        std::vector<std::string> segments;
        std::string current;
        bool inQuotes = false;
        for (char c : content) {
            if (c == '"') inQuotes = !inQuotes;
            if (c == ',' && !inQuotes) {
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
        else if (line.find("=") != std::string::npos && line.substr(0, 5) != "print") {
            size_t pos = line.find("=");
            std::string varName = trim(line.substr(0, pos));
            std::string val = trim(line.substr(pos + 1));
            
            PSXType type = (val[0] == '"') ? TYPE_TEXT : TYPE_NUMBER;
            symbolTable[varName] = {type, val};
        }
        // 3. Print Gabungan
        else if (line.substr(0, 5) == "print") {
            std::string content = trim(line.substr(6));
            std::vector<std::string> segments = splitSegments(content);

            for (std::string& seg : segments) {
                if (seg.empty()) continue;

                // Cek jika Literal String "..."
                if (seg[0] == '"' && seg.back() == '"') {
                    std::cout << seg.substr(1, seg.length() - 2);
                }
                // Cek jika Keyword Tipe Data
                else if (seg == "text" || seg == "number" || seg == "decimal" || seg == "bool" || seg == "char") {
                    std::cout << seg;
                }
                // Cek jika itu Variabel yang sudah terdaftar
                else if (symbolTable.count(seg)) {
                    std::string val = symbolTable[seg].value;
                    if (!val.empty() && val[0] == '"') std::cout << val.substr(1, val.length() - 2);
                    else std::cout << val;
                }
                // Jika hanya angka/teks biasa
                else {
                    std::cout << seg;
                }
                std::cout << " "; // Spasi antar segmen
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