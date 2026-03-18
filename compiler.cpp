#include "compiler.h"
#include <fstream>
#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <sstream>

// Struktur Header ELF tetap sama...
struct ELFHeader {
    uint8_t  e_ident[16] = {0x7F, 'E', 'L', 'F', 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t e_type = 2; uint16_t e_machine = 0x3E; uint32_t e_version = 1;
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

void buildNativeBinary(std::string filename, const std::vector<std::string>& codeLines, bool includeEasterEgg) {
    std::string outputName = filename.substr(0, filename.find_last_of('.')) + ".bin";
    std::ofstream outFile(outputName, std::ios::binary);
    if (!outFile.is_open()) return;

    std::vector<uint8_t> machineCode;

    // Logika easter egg (pesan) sudah dihapus bersih
    
    std::map<std::string, Variable> buildTable; // Tabel simbol sementara untuk compiler

    for (const std::string& line : codeLines) {
        std::string l = trim(line);

        // A. Logika Deklarasi (Agar compiler tahu variabel yang ada)
        if (l.find("::") != std::string::npos) {
            size_t pos = l.find("::");
            std::string typeStr = trim(l.substr(0, pos));
            std::string vars = l.substr(pos + 2);
            PSXType type = (typeStr == "text") ? TYPE_TEXT : 
                           (typeStr == "complex") ? TYPE_COMPLEX : 
                           (typeStr == "null") ? TYPE_NULL : 
                           (typeStr == "decimal") ? TYPE_DECIMAL : TYPE_NUMBER;
            std::stringstream ss(vars);
            std::string v;
            while (std::getline(ss, v, ',')) buildTable[trim(v)] = {type, ""};
        }
        // B. Logika Assignment (Agar compiler tahu isi nilai variabel)
        else if (l.find("=") != std::string::npos && (l.length() < 5 || l.substr(0, 5) != "print")) {
            size_t pos = l.find("=");
            std::string varName = trim(l.substr(0, pos));
            std::string val = trim(l.substr(pos + 1));
            PSXType type = detectType(val); // Gunakan fungsi deteksi pintar
            buildTable[varName] = {type, val};
        }
        // C. Logika Print Gabungan (Baru)
        else if (l.substr(0, 5) == "print") {
            std::string content = trim(l.substr(6));
            std::vector<std::string> segments = splitSegments(content);

            for (std::string& seg : segments) {
                if (seg.empty()) continue;
                std::string textToPrint = "";

                // 1. Cek syntax Label(tipe, variabel)
                if (seg.length() > 7 && seg.substr(0, 6) == "Label(" && seg.back() == ')') {
                    size_t commaPos = seg.find(',');
                    if (commaPos != std::string::npos) {
                        std::string targetType = trim(seg.substr(6, commaPos - 6));
                        std::string varName = trim(seg.substr(commaPos + 1, seg.length() - commaPos - 2));

                        if (buildTable.count(varName)) {
                            Variable var = buildTable[varName];
                            // HAPUS BAGIAN bool match LAMA DAN GANTI INI:
                            bool match = (targetType == "text" && var.type == TYPE_TEXT) ||
                                         (targetType == "number" && var.type == TYPE_NUMBER) ||
                                         (targetType == "decimal" && var.type == TYPE_DECIMAL) ||
                                         (targetType == "bool" && var.type == TYPE_BOOL) ||
                                         (targetType == "char" && var.type == TYPE_CHAR) ||
                                         (targetType == "complex" && var.type == TYPE_COMPLEX) || // Tambahan
                                         (targetType == "null" && var.type == TYPE_NULL);         // Tambahan
                            if (match) {
                                textToPrint = "[" + targetType + ": " + (var.value.front() == '"' ? var.value.substr(1, var.value.length()-2) : var.value) + "] ";
                            } else {
                                textToPrint = "[TypeError: '" + varName + "' bukan " + targetType + "] ";
                            }
                        } else {
                            textToPrint = "[Error: Variabel tidak ditemukan] ";
                        }
                    }
                }
                // 2. Cek jika Literal String "..."
                else if (seg.front() == '"' && seg.back() == '"') {
                    textToPrint = seg.substr(1, seg.length() - 2) + " ";
                }
                // 3. Cek jika itu Variabel biasa (tanpa label)
                else if (buildTable.count(seg)) {
                    std::string val = buildTable[seg].value;
                    if (!val.empty() && val.front() == '"') textToPrint = val.substr(1, val.length() - 2) + " ";
                    else textToPrint = (val.empty() ? "null" : val) + " ";
                }
                // 4. Teks biasa/angka
                else {
                    textToPrint = seg + " ";
                }

                if (!textToPrint.empty()) {
                    uint32_t len = textToPrint.length();
                    // Masukkan ke biner (Syscall Write)
                    uint8_t op1[] = {0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x48, 0xC7, 0xC7, 0x01, 0x00, 0x00, 0x00};
                    for(auto b: op1) machineCode.push_back(b);
                    uint8_t op2[] = {0x48, 0xC7, 0xC2, (uint8_t)len, 0x00, 0x00, 0x00, 0xE8, (uint8_t)len, 0x00, 0x00, 0x00};
                    for(auto b: op2) machineCode.push_back(b);
                    for (char c : textToPrint) machineCode.push_back((uint8_t)c);
                    uint8_t op3[] = {0x5E, 0x0F, 0x05};
                    for(auto b: op3) machineCode.push_back(b);
                }
            }
            // Tambahkan baris baru (newline) di akhir setiap perintah print
            uint8_t nl[] = {0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x48, 0xC7, 0xC7, 0x01, 0x00, 0x00, 0x00, 0x48, 0xC7, 0xC2, 0x01, 0x00, 0x00, 0x00, 0xE8, 0x01, 0x00, 0x00, 0x00, 0x0A, 0x5E, 0x0F, 0x05};
            for(auto b : nl) machineCode.push_back(b);
        }
    }

    uint8_t exitOps[] = {0x48, 0xC7, 0xC0, 0x3C, 0x00, 0x00, 0x00, 0x48, 0xC7, 0xC7, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x05};
    for(auto b : exitOps) machineCode.push_back(b);

    ELFHeader eh; ProgramHeader ph;
    uint64_t totalSize = sizeof(eh) + sizeof(ph) + machineCode.size();
    ph.p_filesz = totalSize; ph.p_memsz = totalSize;
    outFile.write(reinterpret_cast<char*>(&eh), sizeof(eh));
    outFile.write(reinterpret_cast<char*>(&ph), sizeof(ph));
    outFile.write(reinterpret_cast<char*>(machineCode.data()), machineCode.size());
    outFile.close();
    system(("chmod +x " + outputName).c_str());
}