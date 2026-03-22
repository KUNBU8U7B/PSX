#include "compiler.h"
#include "compiler/x86_64.h" 
#include "compiler/arm64.h"
#include <fstream>
#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <sstream>

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

struct ELFHeader {
    uint8_t  e_ident[16] = {0x7f, 'E', 'L', 'F', 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t e_type      = 2;
    uint16_t e_machine   = 0x3E; 
    uint32_t e_version   = 1;
    uint64_t e_entry     = 0x400078;
    uint64_t e_phoff     = 64;
    uint64_t e_shoff     = 0;
    uint32_t e_flags     = 0;
    uint16_t e_ehsize    = 64;
    uint16_t e_phentsize = 56;
    uint16_t e_phnum     = 1;
    uint16_t e_shentsize = 64;
    uint16_t e_shnum     = 0;
    uint16_t e_shstrndx  = 0;
};

struct ProgramHeader {
    uint32_t p_type   = 1;
    uint32_t p_flags  = 5;
    uint64_t p_offset = 0;
    uint64_t p_vaddr  = 0x400000;
    uint64_t p_paddr  = 0x400000;
    uint64_t p_filesz = 0;
    uint64_t p_memsz  = 0;
    uint64_t p_align  = 0x200000;
};

void buildNativeBinary(std::string filename, const std::vector<std::pair<int, std::string>>& codeLines, bool hasUsePsx) {
    std::string outputName = filename.substr(0, filename.find_last_of(".")) + ".bin";
    std::map<std::string, Variable> buildTable;
    std::vector<uint8_t> machineCode;
    
    bool isARM = false; 
#ifdef __aarch64__
    isARM = true;
#endif

    // TANDA BENDERA ERROR (True jika ada minimal 1 error)
    bool hasErrors = false; 

    for (const auto& linePair : codeLines) {
        int lineNum = linePair.first;
        std::string line = linePair.second;

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
                buildTable[trim(varName)] = {type, ""};
            }
        }
        else if (line.find("=") != std::string::npos) {
            size_t pos = line.find("=");
            std::string varName = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));
            
            if (buildTable.count(varName) == 0) {
                // FITUR AUTO-DEKLARASI: Jika variabel belum ada di kompilator, deteksi tipenya otomatis
                PSXType inferredType = detectType(value);
                buildTable[varName] = {inferredType, ""};
            }

            PSXType valType = detectType(value);
            PSXType varType = buildTable[varName].type;

            if (varType != valType && valType != TYPE_NULL) {
                std::cerr << "❌ ERROR (Baris " << lineNum << "): Tipe data tidak cocok! Variabel '" << varName << "' bertipe " << varType << ", tapi diisi dengan " << valType << ".\n";
                hasErrors = true;
                continue;
            }
            buildTable[varName].value = value;
        }
        else if (line.substr(0, 5) == "print") {
            std::string content = line.substr(5);
            std::vector<std::string> segments;
            std::string current;
            bool inQuotes = false;
            int inParens = 0;
            
            // Pemisahan Segmen Print
            for (char c : content) {
                if (c == '"') inQuotes = !inQuotes;
                else if (c == '(' && !inQuotes) inParens++;
                else if (c == ')' && !inQuotes) inParens--;
                if (c == ',' && !inQuotes && inParens == 0) {
                    segments.push_back(trim(current));
                    current = "";
                } else current += c;
            }
            segments.push_back(trim(current));

            for (const std::string& seg : segments) {
                std::string textToPrint = "";
                
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

                        if (!buildTable.count(varName)) {
                            std::cerr << "❌ ERROR (Baris " << lineNum << "): Variabel '" << varName << "' tidak ditemukan!\n";
                            hasErrors = true;
                            continue;
                        }
                        
                        if (buildTable[varName].type != expectedType) {
                            std::cerr << "❌ ERROR (Baris " << lineNum << "): [TypeError: '" << varName << "' bukan " << typeStr << "]\n";
                            hasErrors = true;
                            continue;
                        }

                        std::string val = buildTable[varName].value;
                        textToPrint = (val.front() == '"' ? val.substr(1, val.length()-2) : (val.empty() ? "null" : val)) + " ";
                    }
                }
                else if (seg.front() == '"' && seg.back() == '"') {
                    textToPrint = seg.substr(1, seg.length() - 2) + " ";
                }
                else if (buildTable.count(seg)) {
                    std::string val = buildTable[seg].value;
                    textToPrint = (val.front() == '"' ? val.substr(1, val.length()-2) : (val.empty() ? "null" : val)) + " ";
                }
                else textToPrint = seg + " ";

                // Jangan inject code jika sudah ada error (untuk cegah file rusak)
                if (!textToPrint.empty() && !hasErrors) { 
                    if (isARM) ARM64::injectPrint(machineCode, textToPrint);
                    else X86_64::injectPrint(machineCode, textToPrint);
                }
            }
            if (!hasErrors) {
                if (isARM) ARM64::injectNewline(machineCode);
                else X86_64::injectNewline(machineCode);
            }
        }
    }

    // TAHAP VALIDASI: Hentikan pembuatan biner jika terdeteksi error di atas
    if (hasErrors) {
        std::cerr << "\n🚨 Biner tidak dibuat. Harap perbaiki error di atas terlebih dahulu.\n";
        return; 
    }

    if (isARM) ARM64::injectExit(machineCode);
    else X86_64::injectExit(machineCode);

    ELFHeader eh; ProgramHeader ph;
    eh.e_machine = isARM ? ARM64::MACHINE_ID : X86_64::MACHINE_ID; 

    uint64_t totalSize = sizeof(eh) + sizeof(ph) + machineCode.size();
    ph.p_filesz = totalSize; ph.p_memsz = totalSize;
    
    std::ofstream outFile(outputName, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "❌ Gagal membuat file: " << outputName << std::endl;
        return;
    }

    outFile.write(reinterpret_cast<char*>(&eh), sizeof(eh));
    outFile.write(reinterpret_cast<char*>(&ph), sizeof(ph));
    outFile.write(reinterpret_cast<char*>(machineCode.data()), machineCode.size());
    outFile.close();
    
    std::string chmodCmd = "chmod +x " + outputName;
    system(chmodCmd.c_str());
}