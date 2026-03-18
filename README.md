=========================================================
          DOKUMENTASI BAHASA PEMROGRAMAN PSX
=========================================================
Versi: 1.1 (Tahap 1: Dasar, Complex, & Null)
Author: [Nama Kamu/Username GitHub]
Deskripsi: Bahasa hibrida Interpreter & Native Compiler.

---------------------------------------------------------
1. INISIALISASI (Wajib)
---------------------------------------------------------
Setiap file yang tidak menggunakan ekstensi .psx wajib menggunakan 
perintah berikut di baris pertama agar dikenali oleh engine:

Syntax: use psx
Kegunaan: Mengaktifkan mode pembacaan PSX pada file non-native.

---------------------------------------------------------
2. DEKLARASI VARIABEL
---------------------------------------------------------
PSX mendukung dua cara pembuatan variabel:

A. Deklarasi Otomatis (Implicit)
   Membuat variabel langsung dengan memberikan nilai (Gaya Python).
   Syntax: [nama_variabel] = [nilai]
   Contoh: skor = 100

B. Deklarasi Manual (Explicit)
   Menentukan tipe data di awal sebelum mengisi nilai (Gaya C++).
   Syntax: [tipe_data]:: [nama_variabel1], [nama_variabel2]
   Contoh: number:: x, y
           complex:: arus

---------------------------------------------------------
3. TIPE DATA YANG DIDUKUNG
---------------------------------------------------------
Berdasarkan engine saat ini, PSX mengenali tipe data berikut:

- text     : Data string, wajib diapit kutip dua ("). 
             Contoh: "Halo Dunia"
- number   : Bilangan bulat (integer). 
             Contoh: 10, -5
- decimal  : Bilangan pecahan (float/double). 
             Contoh: 3.14
- bool     : Logika boolean. 
             Nilai: true, false
- char     : Karakter tunggal, wajib diapit kutip satu ('). 
             Contoh: 'A'
- complex  : Bilangan imajiner/kompleks. Menggunakan akhiran 'j'.
             Syntax: [real]+[imaginary]j (Tanpa spasi)
             Contoh: 3+5j, 10-2j
- null     : Tipe data kosong atau reset variabel.
             Keyword: none

---------------------------------------------------------
4. SYNTAX OUTPUT (PRINT)
---------------------------------------------------------
Syntax 'print' di PSX sangat fleksibel dan bisa menggabungkan 
beberapa segmen sekaligus menggunakan koma sebagai pemisah.

A. Print Teks Literal
   print "Teks ini akan muncul di layar"

B. Print Isi Variabel (Tanpa Label)
   nama = "Dharma"
   print nama

C. Print dengan Label (Type Checker)
   Memastikan variabel memiliki tipe data yang benar sebelum dicetak.
   Syntax: Label([target_tipe], [nama_variabel])
   Contoh: print "Skor: ", Label(number, skor)

   *Jika tipe tidak cocok, PSX akan menampilkan [TypeError].
   *Jika variabel tidak ada, PSX akan menampilkan [Error: Variabel tidak ditemukan].

D. Print Gabungan (Multi-Segment)
   print "User: " nama ", Skor: " Label(number, skor)

---------------------------------------------------------
5. MODE KOMPILASI (NATIVE BINARY)
---------------------------------------------------------
Syntax: !compile
Kegunaan: Jika perintah ini ditemukan di dalam file, PSX tidak 
          hanya menjalankan kode tersebut (interpret), tetapi 
          juga membungkus instruksi tersebut menjadi file 
          biner native (.bin) yang bisa dijalankan langsung 
          di sistem operasi Linux (Executable ELF).

---------------------------------------------------------
6. KOMENTAR
---------------------------------------------------------
Syntax: # [teks komentar]
Kegunaan: Menambahkan catatan dalam kode. Baris yang dimulai 
          dengan '#' akan diabaikan oleh engine.

---------------------------------------------------------
7. CONTOH KODE LENGKAP
---------------------------------------------------------
use psx

# Deklarasi
text:: nama
nama = "Dharma"
z = 3+5j
status = none

# Output
print "Nama Pengguna: " nama
print "Bilangan Kompleks: ", Label(complex, z)
print "Status Awal: ", Label(null, status)

!compile
---------------------------------------------------------
