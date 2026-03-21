import subprocess
import os

# Konfigurasi
COMPILER_CMD = ["g++", "main.cpp", "compiler.cpp", "-o", "psx"]
# Daftar file uji coba kamu (tambahkan semua di sini)
TEST_FILES = ["test_file/1.psx", "test_file/3.c", "test_file/2.txt", "test_file/4.psx"] 

def run_command(cmd):
    """Menjalankan perintah dan menangkap output stdout & stderr"""
    result = subprocess.run(cmd, capture_output=True, text=True)
    return result

def main():
    print("--- 🚀 PSX UNIVERSAL TESTER ---")

    # 1. Build Compiler Utama
    print(f"[*] Menyiapkan compiler PSX...", end=" ", flush=True)
    compile_res = run_command(COMPILER_CMD)
    if compile_res.returncode != 0:
        print("❌ GAGAL BENTUK")
        print(compile_res.stderr)
        return
    print("✅ SIAP\n")

    # 2. Proses Setiap File
    for file_name in TEST_FILES:
        if not os.path.exists(file_name):
            print(f"[!] Lewati: {file_name} (File tidak ditemukan)")
            continue

        ext = os.path.splitext(file_name)[1].upper()
        print(f"{'='*10} MENGUJI: {file_name} ({ext}) {'='*10}")

        # JALANKAN INTERPRETER (Selalu diperlihatkan outputnya)
        print(f"[*] Output Interpreter:")
        res = run_command(["./psx", file_name])
        
        # Tampilkan output standar jika ada
        if res.stdout:
            print(res.stdout.strip())
        
        # Tampilkan error jika ada (Misal: lupa 'use psx' di file .c)
        if res.stderr:
            print(f"⚠️  Pesan Sistem/Error:\n{res.stderr.strip()}")

        # 3. CEK LOGIKA KOMPILASI (Hanya jika ada !compile)
        try:
            with open(file_name, 'r') as f:
                content = f.read()
                if "!compile" in content:
                    print(f"\n[*] Mendeteksi '!compile', mengecek biner...")
                    bin_name = os.path.splitext(file_name)[0] + ".bin"
                    
                    if os.path.exists(bin_name):
                        print(f"✅ Biner '{bin_name}' berhasil dibuat.")
                        print(f"[*] Menjalankan hasil biner:")
                        bin_res = run_command([f"./{bin_name}"])
                        print(f"   > {bin_res.stdout.strip()}")
                    else:
                        print(f"❌ Gagal membuat biner.")
        except Exception as e:
            print(f"❌ Gagal membaca file: {e}")
            
        print(f"{'='*40}\n")

    print("--- ✅ Semua Uji Coba Selesai ---")

if __name__ == "__main__":
    main()

