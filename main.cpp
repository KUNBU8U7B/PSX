#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "compiler.h"

// Fungsi untuk menghapus komentar tapi mengabaikan '#' di dalam string ("")
std::string stripComments(const std::string& line) {
    bool inQuotes = false;
    std::string result = "";
    for (size_t i = 0; i < line.length(); ++i) {
        if (line[i] == '"') inQuotes = !inQuotes;
        if (line[i] == '#' && !inQuotes) {
            break; // Berhenti membaca jika ketemu # di luar tanda kutip
        }
        result += line[i];
    }
    return trim(result);
}

class Interpreter {
private:
    std::map<std::string, Variable> symbolTable;

    std::vector<std::string> splitSegments(std::string content) {
        std::vector<std::string> segments;
        std::string current;
        bool inQuotes = false;
        int inParens = 0;
        for (char c : content) {
            if (c == '"') inQuotes = !inQuotes;
            else if (c == '(' && !inQuotes) inParens++;
            else if (c == ')' && !inQuotes) inParens--;
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
    bool execute(int lineNum, std::string line) {
        if (line.empty() || line == "use psx") return true;

        if (line.find("::") != std::string::npos) {
            size_t pos = line.find("::");
            std::string typeStr = trim(line.substr(0, pos));
            std::string vars = line.substr(pos + 2);
            PSXType type = (typeStr == "text") ? TYPE_TEXT : (typeStr == "number") ? TYPE_NUMBER :
                           (typeStr == "decimal") ? TYPE_DECIMAL : (typeStr == "bool") ? TYPE_BOOL :
                           (typeStr == "char") ? TYPE_CHAR : (typeStr == "complex") ? TYPE_COMPLEX : TYPE_UNKNOWN;

            std::stringstream ss(vars);
            std::string varName;
            while (std::getline(ss, varName, ',')) {
                symbolTable[trim(varName)] = {type, ""};
            }
        }
        else if (line.find("=") != std::string::npos) {
            size_t pos = line.find("=");
            std::string varName = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));
            
            if (symbolTable.count(varName) == 0) {
                // FITUR AUTO-DEKLARASI: Jika variabel belum ada, deteksi tipenya otomatis
                PSXType inferredType = detectType(value);
                symbolTable[varName] = {inferredType, ""}; 
            }

            PSXType valType = detectType(value);
            PSXType varType = symbolTable[varName].type;

            if (varType != valType && valType != TYPE_NULL) {
                std::cerr << "❌ ERROR (Baris " << lineNum << "): Tipe data tidak cocok untuk '" << varName << "'.\n";
                return false;
            }
            symbolTable[varName].value = value;
        }
        else if (line.substr(0, 5) == "print") {
            std::string content = line.substr(5);
            std::vector<std::string> segments = splitSegments(content);

            for (std::string seg : segments) {
                if (seg.substr(0, 5) == "Label") {
                    size_t start = seg.find("(");
                    size_t end = seg.find(")");
                    if (start != std::string::npos && end != std::string::npos) {
                        std::string inner = seg.substr(start + 1, end - start - 1);
                        size_t commaPos = inner.find(",");
                        std::string typeStr = trim(inner.substr(0, commaPos));
                        std::string varName = trim(inner.substr(commaPos + 1));

                        PSXType expectedType = (typeStr == "text") ? TYPE_TEXT : (typeStr == "number") ? TYPE_NUMBER :
                                               (typeStr == "decimal") ? TYPE_DECIMAL : (typeStr == "bool") ? TYPE_BOOL :
                                               (typeStr == "char") ? TYPE_CHAR : (typeStr == "complex") ? TYPE_COMPLEX : TYPE_UNKNOWN;

                        if (!symbolTable.count(varName)) {
                            std::cerr << "❌ ERROR (Baris " << lineNum << "): Variabel '" << varName << "' tidak ditemukan!\n";
                            return false;
                        }
                        if (symbolTable[varName].type != expectedType) {
                            std::cerr << "❌ ERROR (Baris " << lineNum << "): [TypeError: '" << varName << "' bukan " << typeStr << "]\n";
                            return false;
                        }

                        std::string val = symbolTable[varName].value;
                        std::cout << (val.front() == '"' ? val.substr(1, val.length()-2) : (val.empty() ? "null" : val));
                    }
                }
                else if (symbolTable.count(seg)) {
                    std::string val = symbolTable[seg].value;
                    std::cout << (val.front() == '"' ? val.substr(1, val.length()-2) : (val.empty() ? "null" : val));
                }
                else std::cout << seg;
                std::cout << " ";
            }
            std::cout << std::endl;
        }
        return true;
    }

    void loadFile(std::string filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return;
        
        std::vector<std::pair<int, std::string>> allLines; 
        std::string line;
        bool hasUsePsx = false, compileRequested = false;
        int lineCount = 0;

        while (std::getline(file, line)) {
            lineCount++; // Hitung baris DARI AWAL, jangan di skip dulu
            
            // Bersihkan komentar & spasi
            std::string cleanLine = stripComments(line);
            
            if (cleanLine.empty()) continue; 
            if (cleanLine == "use psx") { hasUsePsx = true; continue; }
            if (cleanLine == "!compile") { compileRequested = true; continue; }
            
            allLines.push_back({lineCount, cleanLine});
        }
        file.close();

        if ((filename.substr(filename.find_last_of(".") + 1) != "psx") && !hasUsePsx) {
            std::cerr << "❌ ERROR: File non-.psx wajib menggunakan 'use psx'!" << std::endl;
            return;
        }

        if (compileRequested) {
            buildNativeBinary(filename, allLines, hasUsePsx);
        } else { 
            for (const auto& l : allLines) {
                if (!execute(l.first, l.second)) {
                    std::cerr << "⚠️ Eksekusi dihentikan secara paksa untuk melindungi memori.\n";
                    break;
                }
            } 
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc > 1) {
        Interpreter interp;
        interp.loadFile(argv[1]);
    } else {
        std::cout << "Gunakan: ./psx <nama_file.psx>" << std::endl;
    }
    return 0;
}