#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 1. DEFINISI TIPE DATA
// ============================================================================
typedef enum { T_TEXT, T_NUMBER, T_DECIMAL, T_BOOL } VarType;
typedef struct {
    VarType type;
    union {
        char *s_val;
        int i_val;
        double f_val;
        int b_val;
    } data;
} Value;

// ============================================================================
// 2. HASH MAP UNTUK VARIABEL
// ============================================================================
#define HASH_SIZE 256
typedef struct VarNode {
    char nama[64];
    Value val;
    struct VarNode *next;
} VarNode;

VarNode *kamus_hash[HASH_SIZE] = {NULL};

typedef enum {
    PSX_OK = 0,
    PSX_ERR_INPUT,
    PSX_ERR_MATH
} ErrorCode;

ErrorCode status_terakhir = PSX_OK;

// ============================================================================
// 3. DATA FUNGSI & STACK
// ============================================================================
typedef struct {
    char nama[64];
    char parameter[64];
    int baris_mulai;
    int indent_level;
} FungsiPSX;

FungsiPSX daftar_fungsi[50];
int total_fungsi = 0;
int stack_panggil[50];
int top_panggil = -1;
char stack_dest[50][64];
int top_dest = -1;
int stack_loop[50];
int top_loop = -1;

// Variabel global untuk return dan if-elif-else
int psx_return_flag = 0;
int rantai_if_terpenuhi = 0;
int result_sudah_dicopy = 0;

// ============================================================================
// 4. FUNGSI HASH
// ============================================================================
unsigned int hash_func(char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % HASH_SIZE;
}

void psx_set_var(char *nama, Value new_val) {
    unsigned int index = hash_func(nama);
    VarNode *node = kamus_hash[index];
    while (node != NULL) {
        if (strcmp(node->nama, nama) == 0) {
            if (node->val.type == T_TEXT && node->val.data.s_val != NULL) {
                free(node->val.data.s_val);
            }
            node->val = new_val;
            return;
        }
        node = node->next;
    }
    VarNode *new_node = malloc(sizeof(VarNode));
    strncpy(new_node->nama, nama, 63);
    new_node->nama[63] = '\0';
    new_node->val = new_val;
    new_node->next = kamus_hash[index];
    kamus_hash[index] = new_node;
}

Value *psx_get_var(char *nama_dicari) {
    unsigned int index = hash_func(nama_dicari);
    VarNode *node = kamus_hash[index];
    while (node != NULL) {
        if (strcmp(node->nama, nama_dicari) == 0) {
            return &(node->val);
        }
        node = node->next;
    }
    return NULL;
}

// ============================================================================
// 5. FUNGSI EVALUASI MATEMATIKA
// ============================================================================
double evaluasi_ekspresi(char **str);
double evaluasi_faktor(char **str);
double evaluasi_pangkat(char **str);
double evaluasi_perkalian(char **str);

double evaluasi_faktor(char **str) {
    while (**str && isspace((unsigned char)**str)) {
        (*str)++;
    }
    if (strncmp(*str, "sqrt", 4) == 0) {
        *str += 4;
        while (**str && isspace((unsigned char)**str)) {
            (*str)++;
        }
        return sqrt(evaluasi_ekspresi(str));
    }
    if (**str == '(') {
        (*str)++;
        double v = evaluasi_ekspresi(str);
        while (**str && isspace((unsigned char)**str)) {
            (*str)++;
        }
        if (**str == ')') {
            (*str)++;
        }
        return v;
    }
    char *end;
    double val = strtod(*str, &end);
    if (*str == end) {
        while (**str && isspace((unsigned char)**str)) {
            (*str)++;
        }
        char nama_var[64];
        int i = 0;
        while (**str && (isalnum((unsigned char)**str) || **str == '_') && i < 63) {
            nama_var[i++] = *(*str)++;
        }
        nama_var[i] = '\0';
        if (i > 0) {
            Value *v = psx_get_var(nama_var);
            if (v) {
                if (v->type == T_DECIMAL) return v->data.f_val;
                if (v->type == T_NUMBER) return (double)v->data.i_val;
                if (v->type == T_BOOL) return (double)v->data.b_val;
            }
        }
        return 0;
    }
    *str = end;
    return val;
}

double evaluasi_pangkat(char **str) {
    double n = evaluasi_faktor(str);
    while (1) {
        while (**str && isspace((unsigned char)**str)) {
            (*str)++;
        }
        if (**str != '^') {
            break;
        }
        (*str)++;
        while (**str && isspace((unsigned char)**str)) {
            (*str)++;
        }
        double eksponen = evaluasi_pangkat(str);
        n = pow(n, eksponen);
    }
    return n;
}

double evaluasi_perkalian(char **str) {
    double n = evaluasi_pangkat(str);
    while (1) {
        while (**str && isspace((unsigned char)**str)) {
            (*str)++;
        }
        if (**str != '*' && **str != '/' && **str != '%') {
            break;
        }
        char op = *(*str)++;
        while (**str && isspace((unsigned char)**str)) {
            (*str)++;
        }
        double n2 = evaluasi_pangkat(str);
        if (op == '*') {
            n *= n2;
        } else if (op == '/') {
            if (n2 != 0) {
                n = n / n2;
            } else {
                status_terakhir = PSX_ERR_MATH;
                n = 0;
            }
        } else if (op == '%') {
            int pembilang = (int)n;
            int penyebut = (int)n2;
            if (penyebut == 0) {
                status_terakhir = PSX_ERR_MATH;
                n = 0;
            } else {
                n = (double)(pembilang % penyebut);
            }
        }
    }
    return n;
}

double evaluasi_ekspresi(char **str) {
    double n = evaluasi_perkalian(str);
    while (1) {
        while (**str && isspace((unsigned char)**str)) {
            (*str)++;
        }
        if (**str != '+' && **str != '-') {
            break;
        }
        char op = *(*str)++;
        while (**str && isspace((unsigned char)**str)) {
            (*str)++;
        }
        double n2 = evaluasi_perkalian(str);
        if (op == '+') {
            n += n2;
        } else {
            n -= n2;
        }
    }
    return n;
}

// ============================================================================
// 6. FUNGSI PRINT & INPUT
// ============================================================================
void proses_print(char *argumen) {
    char copy_arg[1024];
    strncpy(copy_arg, argumen, 1023);
    copy_arg[1023] = '\0';
    char *token = strtok(copy_arg, ",");
    while (token != NULL) {
        while (isspace((unsigned char)*token)) {
            token++;
        }
        if (token[0] == '"') {
            char pesan[1024];
            if (sscanf(token, "\"%[^\"]\"", pesan) == 1) {
                for (int i = 0; pesan[i] != '\0'; i++) {
                    if (pesan[i] == '\\' && pesan[i+1] == 'n') {
                        printf("\n");
                        i++;
                    } else {
                        putchar(pesan[i]);
                    }
                }
            }
        } else {
            char var_name[64];
            sscanf(token, "%63s", var_name);
            Value *v = psx_get_var(var_name);
            if (v && v->type == T_TEXT) {
                printf("%s", v->data.s_val);
            } else {
                char *ptr = token;
                double hasil = evaluasi_ekspresi(&ptr);
                if (hasil == (long)hasil) {
                    printf("%ld", (long)hasil);
                } else {
                    printf("%g", hasil);
                }
            }
        }
        token = strtok(NULL, ",");
        if (token != NULL) {
            printf(" ");
        }
    }
    printf("\n");
}

// ← PERBAIKAN 1: Prompt DULU, baru clear buffer
void proses_input_strict(char *nama_var, char *pesan, VarType target_type) {
    char buffer[1024];
    
    // 1. TAMPILKAN PROMPT TERLEBIH DAHULU
    printf("%s", pesan);
    fflush(stdout);
    
    // 2. BARU CLEAR BUFFER (setelah user lihat prompt)
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    
    // 3. BACA INPUT
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        status_terakhir = PSX_ERR_INPUT;
        return;
    }
    buffer[strcspn(buffer, "\r\n")] = 0;
    
    char *endptr;
    double d_val = strtod(buffer, &endptr);
    int is_numeric = (strlen(buffer) > 0 && (*endptr == '\0' || isspace((unsigned char)*endptr)));
    int has_dot = (strchr(buffer, '.') != NULL);
    
    Value *existing = psx_get_var(nama_var);
    VarType final_target = (existing != NULL) ? existing->type : target_type;
    
    Value new_val;
    int valid = 0;
    
    if (final_target == T_NUMBER && is_numeric && !has_dot) {
        new_val.type = T_NUMBER;
        new_val.data.i_val = (int)(d_val + 1e-9);
        valid = 1;
    } else if (final_target == T_DECIMAL && is_numeric) {
        new_val.type = T_DECIMAL;
        new_val.data.f_val = d_val;
        valid = 1;
    } else if (final_target == T_TEXT) {
        new_val.type = T_TEXT;
        new_val.data.s_val = strdup(buffer);
        valid = 1;
    }
    
    if (valid) {
        psx_set_var(nama_var, new_val);
        status_terakhir = PSX_OK;
    } else {
        status_terakhir = PSX_ERR_INPUT;
    }
}

int evaluasi_kondisi(char *kondisi) {
    char *ptr = kondisi;
    while (*ptr && isspace((unsigned char)*ptr)) {
        ptr++;
    }
    double kiri = evaluasi_ekspresi(&ptr);
    while (*ptr && isspace((unsigned char)*ptr)) {
        ptr++;
    }
    char op[3] = {0};
    if (strncmp(ptr, "<=", 2) == 0) {
        strcpy(op, "<=");
        ptr += 2;
    } else if (strncmp(ptr, ">=", 2) == 0) {
        strcpy(op, ">=");
        ptr += 2;
    } else if (strncmp(ptr, "==", 2) == 0) {
        strcpy(op, "==");
        ptr += 2;
    } else if (strncmp(ptr, "!=", 2) == 0) {
        strcpy(op, "!=");
        ptr += 2;
    } else if (*ptr == '<') {
        op[0] = '<';
        ptr++;
    } else if (*ptr == '>') {
        op[0] = '>';
        ptr++;
    } else {
        return 0;
    }
    while (*ptr && isspace((unsigned char)*ptr)) {
        ptr++;
    }
    double kanan = evaluasi_ekspresi(&ptr);
    if (strcmp(op, "<") == 0) return kiri < kanan;
    if (strcmp(op, "<=") == 0) return kiri <= kanan;
    if (strcmp(op, ">") == 0) return kiri > kanan;
    if (strcmp(op, ">=") == 0) return kiri >= kanan;
    if (strcmp(op, "==") == 0) return fabs(kiri - kanan) < 1e-9;
    if (strcmp(op, "!=") == 0) return fabs(kiri - kanan) > 1e-9;
    return 0;
}

// ============================================================================
// 7. FUNGSI BANTU INDENTASI
// ============================================================================
int hitung_indentasi(char *s) {
    int indent = 0;
    int i = 0;
    while (s[i] == ' ' || s[i] == '\t') {
        if (s[i] == ' ') {
            indent++;
        } else if (s[i] == '\t') {
            indent += 4;
        }
        i++;
    }
    if (s[i] == '\n' || s[i] == '\r' || s[i] == '\0' || s[i] == '#') {
        return 999;
    }
    return indent;
}

int get_next_real_indent(int start_i, int total_baris, char baris[][512], int indent[]) {
    for (int j = start_i + 1; j < total_baris; j++) {
        if (indent[j] != 999) {
            return indent[j];
        }
    }
    return -1;
}

// ============================================================================
// 8. INTERPRETER PERINTAH
// ============================================================================
void eksekusi_psx(char *baris) {
    while (isspace((unsigned char)*baris)) {
        baris++;
    }
    if (*baris == 0 || *baris == '#') {
        return;
    }
    baris[strcspn(baris, "\r\n")] = 0;
    
    if (strncmp(baris, "return", 6) == 0) {
        char *ptr = baris + 6;
        while (isspace((unsigned char)*ptr)) {
            ptr++;
        }
        if (*ptr != '\0' && *ptr != ';') {
            double hasil = evaluasi_ekspresi(&ptr);
            Value v_ret;
            v_ret.type = T_DECIMAL;
            v_ret.data.f_val = hasil;
            psx_set_var("RESULT", v_ret);
        }
        psx_return_flag = 1;
        return;
    }
    
    if (strncmp(baris, "print ", 6) == 0) {
        proses_print(baris + 6);
        return;
    }
    
    char *cek_sama_dengan = strchr(baris, '=');
    if (cek_sama_dengan != NULL) {
        if (cek_sama_dengan > baris && *(cek_sama_dengan - 1) == '!') {
            return;
        }
        if (cek_sama_dengan > baris && *(cek_sama_dengan - 1) == '<') {
            return;
        }
        if (cek_sama_dengan > baris && *(cek_sama_dengan - 1) == '>') {
            return;
        }
        char n[64];
        int len_nama = cek_sama_dengan - baris;
        if (len_nama > 63) {
            len_nama = 63;
        }
        strncpy(n, baris, len_nama);
        n[len_nama] = '\0';
        char *end_nama = n + strlen(n) - 1;
        while (end_nama >= n && isspace((unsigned char)*end_nama)) {
            *end_nama = '\0';
            end_nama--;
        }
        char *v_str = cek_sama_dengan + 1;
        while (isspace((unsigned char)*v_str)) {
            v_str++;
        }
        
        if (strstr(v_str, "input") != NULL) {
            char pesan_input[512] = "";
            char *start_quote = strchr(v_str, '"');
            if (start_quote) {
                sscanf(start_quote, "\"%511[^\"]\"", pesan_input);
            }
            if (strncmp(v_str, "number(input", 12) == 0) {
                proses_input_strict(n, pesan_input, T_NUMBER);
            } else if (strncmp(v_str, "decimal(input", 13) == 0) {
                proses_input_strict(n, pesan_input, T_DECIMAL);
            } else {
                proses_input_strict(n, pesan_input, T_TEXT);
            }
            return;
        }
        
        Value new_val;
        if (*v_str == '"') {
            new_val.type = T_TEXT;
            char isi[1024];
            sscanf(v_str, "\"%1023[^\"]\"", isi);
            new_val.data.s_val = strdup(isi);
            psx_set_var(n, new_val);
        } else if (strcmp(v_str, "true") == 0 || strcmp(v_str, "false") == 0) {
            new_val.type = T_BOOL;
            new_val.data.b_val = (strcmp(v_str, "true") == 0);
            psx_set_var(n, new_val);
        } else {
            double hasil = evaluasi_ekspresi(&v_str);
            double hasil_aman = hasil + 1e-9;
            Value *existing = psx_get_var(n);
            if (existing) {
                if (existing->type == T_NUMBER) {
                    new_val.type = T_NUMBER;
                    new_val.data.i_val = (int)hasil_aman;
                } else {
                    new_val.type = T_DECIMAL;
                    new_val.data.f_val = hasil;
                }
            } else {
                if (hasil == (long)hasil) {
                    new_val.type = T_NUMBER;
                    new_val.data.i_val = (int)hasil_aman;
                } else {
                    new_val.type = T_DECIMAL;
                    new_val.data.f_val = hasil;
                }
            }
            psx_set_var(n, new_val);
        }
    }
}

// ============================================================================
// 9. MAIN
// ============================================================================
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Gunakan: psx <file.psx>\n");
        return 1;
    }
    
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        printf("Gagal membuka file: %s\n", argv[1]);
        return 1;
    }
    
    static char daftar_baris[2000][512];
    static int indent_baris[2000];
    int total_baris = 0;
    while (fgets(daftar_baris[total_baris], 512, file) && total_baris < 2000) {
        indent_baris[total_baris] = hitung_indentasi(daftar_baris[total_baris]);
        total_baris++;
    }
    fclose(file);
    
    int i = 0;
    int indeks_try = -1;
    int retry_count = 0;
    const int MAX_RETRY = 3;
    
    while (i < total_baris) {
        char *baris_cek = daftar_baris[i];
        int indent_saat_ini = indent_baris[i];
        char *txt = baris_cek;
        while (isspace((unsigned char)*txt)) {
            txt++;
        }
        if (*txt == 0 || *txt == '#') {
            i++;
            continue;
        }
        
        if (strncmp(txt, "func ", 5) == 0) {
            char nama_f[64] = {0}, param_f[64] = {0};
            if (sscanf(txt, "func %63[^(](%63[^)])", nama_f, param_f) >= 1) {
                char *end_n = nama_f + strlen(nama_f) - 1;
                while (end_n >= nama_f && isspace((unsigned char)*end_n)) {
                    *end_n = '\0';
                    end_n--;
                }
                strncpy(daftar_fungsi[total_fungsi].nama, nama_f, 63);
                strncpy(daftar_fungsi[total_fungsi].parameter, param_f, 63);
                daftar_fungsi[total_fungsi].baris_mulai = i;
                daftar_fungsi[total_fungsi].indent_level = indent_saat_ini;
                total_fungsi++;
                i++;
                while (i < total_baris && indent_baris[i] > indent_saat_ini) {
                    i++;
                }
            }
            continue;
        }
        
        char *p_call = strstr(txt, "call ");
        if (p_call != NULL) {
            char var_tujuan[64] = "";
            char *p_sama = strchr(txt, '=');
            if (p_sama != NULL && p_sama < p_call) {
                int len = p_sama - txt;
                if (len > 63) {
                    len = 63;
                }
                strncpy(var_tujuan, txt, len);
                var_tujuan[len] = '\0';
                char *end = var_tujuan + strlen(var_tujuan) - 1;
                while (end > var_tujuan && isspace((unsigned char)*end)) {
                    *end = '\0';
                    end--;
                }
            }
            char nama_f[64] = {0}, arg_f[64] = {0};
            if (sscanf(p_call, "call %63[^(](%63[^)])", nama_f, arg_f) >= 1) {
                char *end_n = nama_f + strlen(nama_f) - 1;
                while (end_n >= nama_f && isspace((unsigned char)*end_n)) {
                    *end_n = '\0';
                    end_n--;
                }
                for (int f = 0; f < total_fungsi; f++) {
                    if (strcmp(daftar_fungsi[f].nama, nama_f) == 0) {
                        Value reset_val;
                        reset_val.type = T_NUMBER;
                        reset_val.data.i_val = 0;
                        psx_set_var("RESULT", reset_val);
                        char param_copy[128], arg_copy[128];
                        strcpy(param_copy, daftar_fungsi[f].parameter);
                        strcpy(arg_copy, arg_f);
                        char *p_tok = strtok(param_copy, ", ");
                        char *a_tok = strtok(arg_copy, ", ");
                        while (p_tok != NULL && a_tok != NULL) {
                            char cmd_p[128];
                            sprintf(cmd_p, "%s = %s", p_tok, a_tok);
                            eksekusi_psx(cmd_p);
                            p_tok = strtok(NULL, ", ");
                            a_tok = strtok(NULL, ", ");
                        }
                        strcpy(stack_dest[++top_dest], var_tujuan);
                        stack_panggil[++top_panggil] = i;
                        i = daftar_fungsi[f].baris_mulai + 1;
                        break;
                    }
                }
            }
            continue;
        }
        
        if (strncmp(txt, "while ", 6) == 0) {
            char kondisi[128];
            sscanf(txt + 6, "%127[^;]", kondisi);
            if (evaluasi_kondisi(kondisi)) {
                stack_loop[++top_loop] = i;
                i++;
            } else {
                i++;
                while (i < total_baris && indent_baris[i] > indent_saat_ini) {
                    i++;
                }
            }
            continue;
        }
        
        int indent_next = get_next_real_indent(i, total_baris, daftar_baris, indent_baris);
        
        if (top_loop >= 0 && indent_next != -1 && indent_next <= indent_baris[stack_loop[top_loop]]) {
            eksekusi_psx(daftar_baris[i]);
            if (psx_return_flag) {
                goto handle_return;
            }
            i = stack_loop[top_loop--];
            continue;
        }
        
        if (top_panggil >= 0 && indent_next != -1 && indent_next <= daftar_fungsi[top_panggil].indent_level) {
            eksekusi_psx(daftar_baris[i]);
            if (psx_return_flag) {
                goto handle_return;
            }
            if (top_dest >= 0 && strlen(stack_dest[top_dest]) > 0 && !result_sudah_dicopy) {
                Value *res = psx_get_var("RESULT");
                if (res) {
                    psx_set_var(stack_dest[top_dest], *res);
                    result_sudah_dicopy = 1;
                }
            }
            if (top_dest >= 0) {
                top_dest--;
            }
            result_sudah_dicopy = 0;
            i = stack_panggil[top_panggil--] + 1;
            continue;
        }
        
        // ← PERBAIKAN 2: If-elif-else dengan skip yang benar
        if (strncmp(txt, "if ", 3) == 0) {
            char kondisi[128];
            sscanf(txt + 3, "%127[^;]", kondisi);
            rantai_if_terpenuhi = evaluasi_kondisi(kondisi);
            if (rantai_if_terpenuhi) {
                i++;
            } else {
                i++;
                while (i < total_baris && indent_baris[i] > indent_saat_ini) {
                    i++;
                }
            }
            continue;
        }
        else if (strncmp(txt, "elif ", 5) == 0) {
            if (rantai_if_terpenuhi) {
                // Sudah ada yang true, skip blok elif ini
                i++;
                while (i < total_baris && indent_baris[i] > indent_saat_ini) {
                    i++;
                }
            } else {
                // Belum ada yang true, cek kondisi ini
                char kondisi[128];
                sscanf(txt + 5, "%127[^;]", kondisi);
                rantai_if_terpenuhi = evaluasi_kondisi(kondisi);
                if (rantai_if_terpenuhi) {
                    i++;
                } else {
                    i++;
                    while (i < total_baris && indent_baris[i] > indent_saat_ini) {
                        i++;
                    }
                }
            }
            continue;
        }
        else if (strncmp(txt, "else", 4) == 0) {
            if (rantai_if_terpenuhi) {
                // Sudah ada yang true, skip blok else
                i++;
                while (i < total_baris && indent_baris[i] > indent_saat_ini) {
                    i++;
                }
            } else {
                // Belum ada yang true, eksekusi else
                i++;
            }
            continue;
        }
        
        // Reset if flag ketika keluar dari blok if-elif-else
        if (indent_next != -1 && indent_next < indent_saat_ini) {
            rantai_if_terpenuhi = 0;
        }
        
        if (strncmp(txt, "try", 3) == 0 && (txt[3] == '\0' || txt[3] == ';' || isspace((unsigned char)txt[3]))) {
            indeks_try = i;
            int indent_induk = indent_baris[i];
            status_terakhir = PSX_OK;
            retry_count = 0;
            i++;
            while (i < total_baris) {
                if (indent_baris[i] <= indent_induk) {
                    break;
                }
                char *cek_kosong = daftar_baris[i];
                while (isspace((unsigned char)*cek_kosong)) {
                    cek_kosong++;
                }
                if (*cek_kosong == 0 || *cek_kosong == '#') {
                    i++;
                    continue;
                }
                if (status_terakhir == PSX_OK) {
                    eksekusi_psx(daftar_baris[i]);
                    if (psx_return_flag) {
                        break;
                    }
                }
                i++;
            }
            if (psx_return_flag) {
                goto handle_return;
            }
            if (status_terakhir != PSX_OK) {
                while (i < total_baris) {
                    char *txt_catch = daftar_baris[i];
                    while (isspace((unsigned char)*txt_catch)) {
                        txt_catch++;
                    }
                    if (*txt_catch == 0 || *txt_catch == '#') {
                        i++;
                        continue;
                    }
                    if (indent_baris[i] == indent_induk && strncmp(txt_catch, "catch", 5) == 0) {
                        int match = 0;
                        if (status_terakhir == PSX_ERR_INPUT && strstr(txt_catch, "INPUT_ERROR")) {
                            match = 1;
                        } else if (status_terakhir == PSX_ERR_MATH && strstr(txt_catch, "MATH_ERROR")) {
                            match = 1;
                        } else if (!strstr(txt_catch, "(")) {
                            match = 1;
                        }
                        if (match) {
                            i++;
                            int harus_ulang = 0;
                            while (i < total_baris) {
                                if (indent_baris[i] <= indent_induk) {
                                    break;
                                }
                                char *baris_clean = daftar_baris[i];
                                while (isspace((unsigned char)*baris_clean)) {
                                    baris_clean++;
                                }
                                if (strncmp(baris_clean, "repeat;", 7) == 0) {
                                    harus_ulang = 1;
                                    break;
                                }
                                eksekusi_psx(daftar_baris[i]);
                                i++;
                            }
                            if (harus_ulang && indeks_try != -1) {
                                retry_count++;
                                if (retry_count >= MAX_RETRY) {
                                    printf("Error: Maksimum retry (%d) tercapai\n", MAX_RETRY);
                                    status_terakhir = PSX_OK;
                                    break;
                                }
                                i = indeks_try;
                                status_terakhir = PSX_OK;
                                break;
                            }
                            status_terakhir = PSX_OK;
                            break;
                        }
                    }
                    if (indent_baris[i] < indent_induk) {
                        break;
                    }
                    i++;
                }
            } else {
                while (i < total_baris) {
                    char *txt_next = daftar_baris[i];
                    while (isspace((unsigned char)*txt_next)) {
                        txt_next++;
                    }
                    if (indent_baris[i] == indent_induk && strncmp(txt_next, "catch", 5) == 0) {
                        i++;
                        while (i < total_baris && indent_baris[i] > indent_induk) {
                            i++;
                        }
                    } else {
                        break;
                    }
                }
            }
            continue;
        }
        
        eksekusi_psx(daftar_baris[i]);
        
        handle_return:
        if (psx_return_flag) {
            if (top_panggil >= 0) {
                if (top_dest >= 0 && strlen(stack_dest[top_dest]) > 0 && !result_sudah_dicopy) {
                    Value *res = psx_get_var("RESULT");
                    if (res) {
                        psx_set_var(stack_dest[top_dest], *res);
                        result_sudah_dicopy = 1;
                    }
                }
                if (top_dest >= 0) {
                    top_dest--;
                }
                result_sudah_dicopy = 0;
                i = stack_panggil[top_panggil--] + 1;
            } else {
                i++;
            }
            psx_return_flag = 0;
            continue;
        }
        i++;
    }
    
    for (int j = 0; j < HASH_SIZE; j++) {
        VarNode *node = kamus_hash[j];
        while (node != NULL) {
            VarNode *temp = node;
            node = node->next;
            if (temp->val.type == T_TEXT && temp->val.data.s_val != NULL) {
                free(temp->val.data.s_val);
            }
            free(temp);
        }
    }
    
    return 0;
}