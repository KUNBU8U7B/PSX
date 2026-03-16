use psx

# ---------------------------------------------------------
# 1. DEKLARASI VARIABEL DENGAN BERBAGAI TIPE
# ---------------------------------------------------------
nama = "Dharma"
umur = 25
pi = 3.14
is_aktif = true
grade = 'A'

# ---------------------------------------------------------
# 2. UJI COBA LABEL (KONDISI BERHASIL)
# ---------------------------------------------------------
print "--- UJI BERHASIL ---"
print "Nama user: ", Label(text, nama)
print "Umur user: ", Label(number, umur)
print "Nilai Pi:  ", Label(decimal, pi)
print "Status:    ", Label(bool, is_aktif)
print "Grade:     ", Label(char, grade)

# ---------------------------------------------------------
# 3. UJI COBA LABEL (KONDISI TYPE ERROR)
# ---------------------------------------------------------
print ""
print "--- UJI TYPE ERROR ---"
# Mencoba memanggil 'umur' (number) sebagai 'text'
print "Tes Error 1: ", Label(text, umur)

# Mencoba memanggil 'nama' (text) sebagai 'number'
print "Tes Error 2: ", Label(number, nama)

# ---------------------------------------------------------
# 4. UJI COBA VARIABEL TIDAK DITEMUKAN
# ---------------------------------------------------------
print ""
print "--- UJI VARIABEL GAIB ---"
print "Tes Error 3: ", Label(text, alamat)


print "End of Test Cases, have a nice day!","PSX is awesome! :) use psx for more fun coding experience!","nice day! :)"
# ---------------------------------------------------------
# 5. KOMPILASI KE BINER
# ---------------------------------------------------------
!compile