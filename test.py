import subprocess
import os

# Konfigurasi WSL
# Path Windows: C:\Users\Pongo\Documents\CODING\C
# Path WSL: /mnt/c/Users/Pongo/Documents/CODING/C
BASE_PATH_WSL = "/mnt/c/Users/Pongo/Documents/CODING/C"

def run_wsl(cmd_list):
    """Menjalankan perintah di dalam WSL dengan encoding UTF-8"""
    wsl_cmd = ["wsl"] + cmd_list
    result = subprocess.run(wsl_cmd, capture_output=True, text=True, encoding='utf-8', errors='replace')
    return result

def safe_print(text):
    """Mencetak teks ke konsol dengan menangani karakter yang tidak didukung"""
    try:
        print(text)
    except UnicodeEncodeError:
        print(text.encode('ascii', 'replace').decode('ascii'))

def main():
    print("--- PSX UNIVERSAL TESTER (WSL MODE) ---")

    # 1. Build Compiler Utama di WSL
    print(f"[*] Menyiapkan compiler PSX di WSL...", end=" ", flush=True)
    compile_cmd = ["g++", f"{BASE_PATH_WSL}/main.cpp", f"{BASE_PATH_WSL}/compiler.cpp", "-o", f"{BASE_PATH_WSL}/psx"]
    compile_res = run_wsl(compile_cmd)
    if compile_res.returncode != 0:
        print("[X] GAGAL BUILD")
        print(compile_res.stderr)
        return
    print("[V] SIAP\n")

    # 2. Daftar file uji coba
    test_dir = "test_file"
    test_files = [f for f in os.listdir(test_dir) if f.endswith(('.psx', '.c', '.txt'))]
    test_files.sort()

    for file_name in test_files:
        full_path_win = os.path.abspath(os.path.join(test_dir, file_name))
        # Konversi path Windows ke WSL path
        file_path_wsl = f"{BASE_PATH_WSL}/test_file/{file_name}"
        
        ext = os.path.splitext(file_name)[1].upper()
        print(f"{'='*10} MENGUJI: {file_name} ({ext}) {'='*10}")

        # JALANKAN INTERPRETER
        safe_print(f"[*] Output Interpreter:")
        res = run_wsl([f"{BASE_PATH_WSL}/psx", file_path_wsl])
        
        if res.stdout:
            safe_print(res.stdout.strip())
        
        if res.stderr:
            safe_print(f"(!) Pesan Sistem/Error:\n{res.stderr.strip()}")

        # 3. CEK LOGIKA KOMPILASI (Hanya jika ada !compile)
        try:
            with open(full_path_win, 'r') as f:
                content = f.read()
                if "!compile" in content:
                    print(f"\n[*] Mendeteksi '!compile', mengecek biner...")
                    bin_name_wsl = f"{BASE_PATH_WSL}/test_file/{os.path.splitext(file_name)[0]}.bin"
                    
                    # Cek keberadaan file di WSL (via ls)
                    check_bin = run_wsl(["ls", bin_name_wsl])
                    if check_bin.returncode == 0:
                        safe_print(f"[V] Biner berhasil dibuat di WSL.")
                        safe_print(f"[*] Menjalankan hasil biner di WSL:")
                        # Pastikan executable
                        run_wsl(["chmod", "+x", bin_name_wsl])
                        bin_res = run_wsl([bin_name_wsl])
                        if bin_res.stdout:
                            safe_print(f"   > {bin_res.stdout.strip()}")
                        if bin_res.stderr:
                            safe_print(f"   ! Error Biner: {bin_res.stderr.strip()}")
                    else:
                        safe_print(f"[X] Gagal membuat biner (Biner tidak ditemukan).")
        except Exception as e:
            safe_print(f"[X] Gagal memproses file: {e}")
            
        print(f"{'='*40}\n")

    print("--- Semua Uji Coba Selesai ---")

if __name__ == "__main__":
    main()

