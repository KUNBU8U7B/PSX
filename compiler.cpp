#include "compiler.h"
#include "compiler/x86_64.h" // Gunakan backend x86_64
#include <fstream>
#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <sstream>

// Implementasi fungsi utilitas (dipindah dari main.cpp)
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

PSXType detectType(std::string val) {
    if (val.empty() || val == "none") return TYPE_NULL;
    if (val.front() == '"' && val.back() == '"') return TYPE_TEXT;
    if (val.front() == '\'' && val.back() == '\'' && val.length() >= 3) return TYPE_CHAR;
    if (val == "true" || val == "false") return TYPE_BOOL;
    if (val.find('j') != std::string::npos && (val.find('+') != std::string::npos || val.find('-') != std::string::npos)) 
        return TYPE_COMPLEX;
    if (val.find('.') != std::string::npos) return TYPE_DECIMAL;
    return TYPE_NUMBER;
}

// Struktur Header ELF
struct ELFHeader {
    uint8_t  e_ident[16] = {0x7F, 'E', 'L', 'F', 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t e_type = 2; 
    uint16_t e_machine = 0; // Diisi dinamis
    uint32_t e_version = 1;
    uint64_t e_entry = 0x400078; uint64_t e_phoff = 64; uint64_t e_shoff = 0;
    uint32_t e_flags = 0; uint16_t e_ehsize = 64; uint16_t e_phentsize = 56;
    uint16_t e_phnum = 1; uint16_t e_shentsize = 0; uint16_t e_shnum = 0; uint16_t e_shstrndx = 0;
};

struct ProgramHeader {
    uint32_t p_type = 1; uint32_t p_flags = 5; uint64_t p_offset = 0;
    uint64_t p_vaddr = 0x400000; uint64_t p_paddr = 0x400000;
    uint64_t p_filesz = 0; uint64_t p_memsz = 0; uint64_t p_align = 0x1000;
};

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

void buildNativeBinary(std::string filename, const std::vector<std::string>& codeLines, bool includeEasterEgg) {
    std::string outputName = filename.substr(0, filename.find_last_of('.')) + ".bin";
    std::ofstream outFile(outputName, std::ios::binary);
    if (!outFile.is_open()) return;

    std::vector<uint8_t> machineCode;
    std::map<std::string, Variable> buildTable;

    for (const std::string& line : codeLines) {
        std::string l = trim(line);

        // A. Deklarasi
        if (l.find("::") != std::string::npos) {
            size_t pos = l.find("::");
            std::string typeStr = trim(l.substr(0, pos));
            std::string vars = l.substr(pos + 2);
            PSXType type = (typeStr == "text") ? TYPE_TEXT : 
                           (typeStr == "complex") ? TYPE_COMPLEX : 
                           (typeStr == "null") ? TYPE_NULL : 
                           (typeStr == "decimal") ? TYPE_DECIMAL : 
                           (typeStr == "bool") ? TYPE_BOOL :
                           (typeStr == "char") ? TYPE_CHAR : TYPE_NUMBER;
            std::stringstream ss(vars);
            std::string v;
            while (std::getline(ss, v, ',')) buildTable[trim(v)] = {type, ""};
        }
        // B. Assignment
        else if (l.find("=") != std::string::npos && (l.length() < 5 || l.substr(0, 5) != "print")) {
            size_t pos = l.find("=");
            std::string varName = trim(l.substr(0, pos));
            std::string val = trim(l.substr(pos + 1));
            buildTable[varName] = {detectType(val), val};
        }
        // C. Print Logic
        else if (l.substr(0, 5) == "print") {
            std::string content = trim(l.substr(6));
            std::vector<std::string> segments = splitSegments(content);

            for (std::string& seg : segments) {
                if (seg.empty()) continue;
                std::string textToPrint = "";

                if (seg.length() > 7 && seg.substr(0, 6) == "Label(" && seg.back() == ')') {
                    size_t commaPos = seg.find(',');
                    if (commaPos != std::string::npos) {
                        std::string targetType = trim(seg.substr(6, commaPos - 6));
                        std::string varName = trim(seg.substr(commaPos + 1, seg.length() - commaPos - 2));

                        if (buildTable.count(varName)) {
                            Variable var = buildTable[varName];
                            bool match = (targetType == "text" && var.type == TYPE_TEXT) ||
                                         (targetType == "number" && var.type == TYPE_NUMBER) ||
                                         (targetType == "decimal" && var.type == TYPE_DECIMAL) ||
                                         (targetType == "bool" && var.type == TYPE_BOOL) ||
                                         (targetType == "char" && var.type == TYPE_CHAR) ||
                                         (targetType == "complex" && var.type == TYPE_COMPLEX) ||
                                         (targetType == "null" && var.type == TYPE_NULL);
                            if (match) {
                                textToPrint = "[" + targetType + ": " + (var.value.front() == '"' ? var.value.substr(1, var.value.length()-2) : var.value) + "] ";
                            } else {
                                textToPrint = "[TypeError: '" + varName + "' bukan " + targetType + "] ";
                            }
                        }
                    }
                }
                else if (seg.front() == '"' && seg.back() == '"') {
                    textToPrint = seg.substr(1, seg.length() - 2) + " ";
                }
                else if (buildTable.count(seg)) {
                    std::string val = buildTable[seg].value;
                    textToPrint = (val.front() == '"' ? val.substr(1, val.length()-2) : (val.empty() ? "null" : val)) + " ";
                }
                else {
                    textToPrint = seg + " ";
                }

                if (!textToPrint.empty()) {
                    X86_64::injectPrint(machineCode, textToPrint);
                }
            }
            X86_64::injectNewline(machineCode);
        }
    }

    X86_64::injectExit(machineCode);

    ELFHeader eh; ProgramHeader ph;
    eh.e_machine = X86_64::MACHINE_ID; // Set ke 0x3E

    uint64_t totalSize = sizeof(eh) + sizeof(ph) + machineCode.size();
    ph.p_filesz = totalSize; ph.p_memsz = totalSize;
    
    outFile.write(reinterpret_cast<char*>(&eh), sizeof(eh));
    outFile.write(reinterpret_cast<char*>(&ph), sizeof(ph));
    outFile.write(reinterpret_cast<char*>(machineCode.data()), machineCode.size());
    outFile.close();
    
    system(("chmod +x " + outputName).c_str());
    std::cout << "✅ Compiled to " << outputName << " (x86_64)" << std::endl;
}