#ifndef ARM64_H
#define ARM64_H

#include <vector>
#include <string>
#include <cstdint>

namespace ARM64 {
    // ID Mesin ARM64 (AArch64) untuk Header ELF
    const uint16_t MACHINE_ID = 0xB7;

    // Fungsi utilitas untuk memasukkan instruksi 32-bit (4 byte) ke vector 8-bit
    inline void pushInst(std::vector<uint8_t>& code, uint32_t inst) {
        code.push_back(inst & 0xFF);
        code.push_back((inst >> 8) & 0xFF);
        code.push_back((inst >> 16) & 0xFF);
        code.push_back((inst >> 24) & 0xFF);
    }

    inline void injectPrint(std::vector<uint8_t>& machineCode, const std::string& text) {
        uint32_t len = text.length();

        // 1. mov x0, #1 (stdout) -> 0xd2800020
        pushInst(machineCode, 0xd2800020);
        
        // 2. adr x1, #16 (alamat teks setelah instruksi syscall)
        // Kita lompat 4 instruksi kedepan untuk ambil data string
        pushInst(machineCode, 0x10000081);

        // 3. mov x2, #len (panjang string)
        uint32_t mov_x2 = 0xd2800002 | ((len & 0xFFFF) << 5);
        pushInst(machineCode, mov_x2);

        // 4. mov x8, #64 (syscall write di ARM64 adalah 64)
        pushInst(machineCode, 0xd2800808);

        // 5. svc #0 (syscall)
        pushInst(machineCode, 0xd4000001);

        // 6. b #len (Lompat melewati data string agar tidak dieksekusi sebagai instruksi)
        uint32_t branch = 0x14000000 | ((len / 4 + 1) & 0x3FFFFFF);
        pushInst(machineCode, branch);

        // 7. Masukkan data string
        for (char c : text) machineCode.push_back((uint8_t)c);
        // Padding agar instruksi berikutnya tetap sejajar 4 byte (alignment)
        while (machineCode.size() % 4 != 0) machineCode.push_back(0);
    }

    inline void injectNewline(std::vector<uint8_t>& machineCode) {
        injectPrint(machineCode, "\n");
    }

    inline void injectExit(std::vector<uint8_t>& machineCode) {
        // mov x0, #0 (exit code 0)
        pushInst(machineCode, 0xd2800000);
        // mov x8, #93 (syscall exit di ARM64 adalah 93)
        pushInst(machineCode, 0xd2800ba8);
        // svc #0
        pushInst(machineCode, 0xd4000001);
    }
}

#endif