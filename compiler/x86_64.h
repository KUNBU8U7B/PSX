#ifndef X86_64_H
#define X86_64_H

#include <vector>
#include <string>
#include <cstdint>

namespace X86_64 {
    // ID Mesin x86_64 untuk Header ELF
    const uint16_t MACHINE_ID = 0x3E;

    // Fungsi untuk menyuntikkan instruksi Print (Syscall Write)
    inline void injectPrint(std::vector<uint8_t>& machineCode, const std::string& text) {
        uint32_t len = text.length();
        
        uint8_t op1[] = {0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x48, 0xC7, 0xC7, 0x01, 0x00, 0x00, 0x00};
        for(auto b: op1) machineCode.push_back(b);
        
        uint8_t op2[] = {0x48, 0xC7, 0xC2, (uint8_t)len, 0x00, 0x00, 0x00, 0xE8, (uint8_t)len, 0x00, 0x00, 0x00};
        for(auto b: op2) machineCode.push_back(b);
        
        for (char c : text) machineCode.push_back((uint8_t)c);
        
        uint8_t op3[] = {0x5E, 0x0F, 0x05};
        for(auto b: op3) machineCode.push_back(b);
    }

    // Fungsi untuk menyuntikkan Baris Baru (Newline)
    inline void injectNewline(std::vector<uint8_t>& machineCode) {
        uint8_t nl[] = {0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x48, 0xC7, 0xC7, 0x01, 0x00, 0x00, 0x00, 0x48, 0xC7, 0xC2, 0x01, 0x00, 0x00, 0x00, 0xE8, 0x01, 0x00, 0x00, 0x00, 0x0A, 0x5E, 0x0F, 0x05};
        for(auto b : nl) machineCode.push_back(b);
    }

    // Fungsi untuk menyuntikkan instruksi Exit (Syscall Exit)
    inline void injectExit(std::vector<uint8_t>& machineCode) {
        uint8_t exitOps[] = {0x48, 0xC7, 0xC0, 0x3C, 0x00, 0x00, 0x00, 0x48, 0xC7, 0xC7, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x05};
        for(auto b : exitOps) machineCode.push_back(b);
    }
}

#endif