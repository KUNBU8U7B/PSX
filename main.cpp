#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "compiler.h"

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
    void execute(std::string line) {
        line = trim(line);
        if (line.empty() || line == "use psx") return;

        if (line.find("::") != std::string::npos) {
            size_t pos = line.find("::");
            std::string typeStr = trim(line.substr(0, pos));
            std::string vars = line.substr(pos + 2);
            PSXType type = (typeStr == "text") ? TYPE_TEXT : (typeStr == "number") ? TYPE_NUMBER : 
                           (typeStr == "decimal") ? TYPE_DECIMAL : (typeStr == "bool") ? TYPE_BOOL :
                           (typeStr == "char") ? TYPE_CHAR : (typeStr == "complex") ? TYPE_COMPLEX : TYPE_NULL;
            std::stringstream ss(vars);
            std::string v;
            while (std::getline(ss, v, ',')) symbolTable[trim(v)] = {type, ""};
        }
        else if (line.find("=") != std::string::npos && (line.length() < 5 || line.substr(0, 5) != "print")) {
            size_t pos = line.find("=");
            std::string varName = trim(line.substr(0, pos));
            std::string val = trim(line.substr(pos + 1));
            symbolTable[varName] = {detectType(val), val};
        }
        else if (line.substr(0, 5) == "print") {
            std::string content = trim(line.substr(6));
            std::vector<std::string> segments = splitSegments(content);
            for (std::string& seg : segments) {
                if (seg.empty()) continue;
                if (seg.length() > 7 && seg.substr(0, 6) == "Label(" && seg.back() == ')') {
                    size_t commaPos = seg.find(',');
                    if (commaPos != std::string::npos) {
                        std::string targetType = trim(seg.substr(6, commaPos - 6));
                        std::string varName = trim(seg.substr(commaPos + 1, seg.length() - commaPos - 2));
                        if (symbolTable.count(varName)) {
                            Variable var = symbolTable[varName];
                            bool match = (targetType == "text" && var.type == TYPE_TEXT) || (targetType == "number" && var.type == TYPE_NUMBER) || (targetType == "decimal" && var.type == TYPE_DECIMAL) || (targetType == "bool" && var.type == TYPE_BOOL) || (targetType == "char" && var.type == TYPE_CHAR) || (targetType == "complex" && var.type == TYPE_COMPLEX) || (targetType == "null" && var.type == TYPE_NULL);
                            if (match) std::cout << "[" << targetType << ": " << (var.value.front() == '"' ? var.value.substr(1, var.value.length()-2) : var.value) << "]";
                            else std::cout << "[TypeError: '" << varName << "' bukan " << targetType << "]";
                        }
                    }
                }
                else if (seg.front() == '"' && seg.back() == '"') std::cout << seg.substr(1, seg.length() - 2);
                else if (symbolTable.count(seg)) {
                    std::string val = symbolTable[seg].value;
                    std::cout << (val.front() == '"' ? val.substr(1, val.length()-2) : (val.empty() ? "null" : val));
                }
                else std::cout << seg;
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
        bool hasUsePsx = false, compileRequested = false;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;
            if (line == "use psx") hasUsePsx = true;
            if (line == "!compile") { compileRequested = true; continue; }
            allLines.push_back(line);
        }
        file.close();
        if ((filename.substr(filename.find_last_of(".") + 1) != "psx") && !hasUsePsx) {
            std::cerr << "❌ ERROR: File non-.psx wajib menggunakan 'use psx'!" << std::endl;
            return;
        }
        if (compileRequested) buildNativeBinary(filename, allLines, hasUsePsx);
        else { for (const std::string& l : allLines) execute(l); }
    }
};

int main(int argc, char* argv[]) {
    Interpreter psx;
    if (argc > 1) psx.loadFile(argv[1]);
    else std::cout << "PSX Mode Terhubung." << std::endl;
    return 0;
}