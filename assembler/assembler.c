#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_UINT16_T 65535

typedef struct {
    char *data;
    size_t len;
} String_View;

#define SV_STATIC(str)                                                         \
(String_View) { .data = str, .len = sizeof(str) - 1 }

bool sv_cmp(const String_View sv1, const String_View sv2) {
    if (sv1.len != sv2.len)
        return false;
    for (int i = 0; i < sv1.len; i++)
        if (sv1.data[i] != sv2.data[i])
            return false;
    return true;
}

bool sv_contains(const String_View sv, const char ch) {
    for (int i = 0; i < sv.len; i++) {
        if (sv.data[i] == ch)
            return true;
    }
    return false;
}

#define SV_FMT "%.*s"
#define SV_ARG(SV) ((int)(SV).len), (SV).data

size_t hash_string_view(String_View sv) {
    size_t hash = 0;
    for (int i = 0; i < sv.len; i++) {
        hash += (sv.data[i] * 10) / 3;
    }
    return hash;
}

typedef struct {
    size_t bytes_count;
    String_View content;
} Label;

Label new_label(String_View content, size_t bytes_count) {
    return (Label){.content = content, .bytes_count = bytes_count};
}

typedef struct SLabel_Element {
    bool is_occupied;
    size_t bytes_count;
    String_View content;
    struct SLabel_Element *next;
} Label_Element;

typedef struct {
    Label_Element *labels;
    size_t capacity;
    size_t count;
} Label_Hashmap;

Label_Hashmap label_hminit(size_t init_cap) {
    Label_Element *labels =
        (Label_Element *)malloc(sizeof(Label_Element) * init_cap);
    return (Label_Hashmap){
        .capacity = init_cap,
        .labels = labels,
    };
}

void insert_label(Label_Hashmap *lhm, const String_View key,
                  const Label value) {
    if (lhm->count + 1 >= lhm->capacity) {
        lhm->capacity *= 2;
        size_t *labels =
            (size_t *)realloc(lhm->labels, sizeof(size_t) * lhm->capacity);
    }
    size_t hash = hash_string_view(key);
    hash %= lhm->capacity;
    Label_Element element = {
        .is_occupied = true,
        .next = NULL,
        .bytes_count = value.bytes_count,
        .content = value.content,
    };
    Label_Element *old_elem = &lhm->labels[hash];
    if (old_elem->is_occupied && old_elem != NULL) {
        while (old_elem->next != NULL) old_elem = old_elem->next;
        Label_Element *new_elem = (Label_Element *)malloc(sizeof(Label_Element));
        old_elem->next = new_elem;
        *new_elem = element;
    } else {
        lhm->labels[hash] = element;
    }
    lhm->count++; 
}

Label get_label(const Label_Hashmap *lhm, const String_View key) {
    size_t hash = hash_string_view(key);
    hash %= lhm->capacity;
    Label_Element elem = lhm->labels[hash];
    if (!elem.is_occupied) {
        return (Label){
            .content.data = NULL,
        };
    } else {
        if (sv_cmp(elem.content, key)) {
            return (Label){
                .content = elem.content,
                .bytes_count = elem.bytes_count,
            };
        }
        for (; elem.next;) {
            if (sv_cmp(elem.content, key)) {
                return (Label){
                    .content = elem.content,
                    .bytes_count = elem.bytes_count,
                };
            } else {
                elem = *elem.next;
            }
        }
    }
    return (Label){
        .content.data = NULL,
    };
}

char *read_file(const char *file_name, size_t *size) {
    FILE *file;
    file = fopen(file_name, "r");

    if (file == NULL)
        return NULL;

    fseek(file, 0, SEEK_END);
    int len = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(sizeof(char) * len);
    for (int i = 0; i < len; i++) {
        content[i] = fgetc(file);
    }
    if (size)
        *size = len;
    return content;
}

typedef struct {
    char *content;
    size_t size;
    size_t cursor;
    size_t lineNo;
    size_t bol;
    char *file_path;
} Lexer;

typedef struct {
    size_t lineNo;
    size_t colNo;
    char *file_path;
} Location;

void print_loc(const Location l) {
    printf("%s:%zu:%zu ", l.file_path, l.lineNo, l.colNo);
}

typedef struct {
    String_View content;
    Location loc;
} String_Token;

Lexer lex_new(char *content, size_t size, char *file_path) {
    return (Lexer){
        .lineNo = 1,
        .content = content,
        .size = size,
        .file_path = file_path,
    };
}

void lex_skip_space(Lexer *l) {
    for (; isspace(l->content[l->cursor]) && l->cursor < l->size; l->cursor++) {
        if (l->content[l->cursor] == '\n') {
            l->lineNo++;
            l->bol = l->cursor;
        }
    }
}

String_Token lex_chop_token(Lexer *l) {
    String_Token st = {0};
    st.loc.file_path = l->file_path;
    String_View sv = {0};
    lex_skip_space(l);
    sv.data = &l->content[l->cursor];
    sv.len = 0;
    for (int i = 0; l->cursor < l->size; i++) {
        if (l->content[l->cursor] == '"') { 
            sv.data = &l->content[l->cursor];
            l->cursor++; 
            sv.len++;
            for (;l->content[l->cursor] != '"';) {
                l->cursor++;
                sv.len++;
            }
            l->cursor++;
            sv.len++;
            st.content = sv;
            st.loc.lineNo = l->lineNo;
            st.loc.colNo = l->bol;
            return st;
        }
        if (isspace(l->content[l->cursor])) {
            if (l->content[l->cursor] == '\n') {
                l->lineNo++;
                l->bol = l->cursor;
            }
            st.content = sv;
            st.loc.lineNo = l->lineNo;
            st.loc.colNo = l->bol;
            return st;
        }
        l->cursor++;
        sv.len++;
    }
    return st;
}

typedef enum {
    TOKEN_ADD,
    TOKEN_AND,
    TOKEN_JMP,
    TOKEN_LD,
    TOKEN_LDI,
    TOKEN_LDR,
    TOKEN_ST,
    TOKEN_STI,
    TOKEN_STR,
    TOKEN_RTI,
    TOKEN_NOT,
    TOKEN_TRAP,
    TOKEN_LEA,
    TOKEN_JSR,
    TOKEN_JSRR,
    TOKEN_RET,
    TOKEN_BR,
    TOKEN_COUNT,

    TOKEN_END,
    TOKEN_ILLEGAL,
    TOKEN_REG,
    TOKEN_LABEL_DEF,
    TOKEN_LABEL_CALL,
    TOKEN_INT_LIT,
    TOKEN_STR_LIT,
    TOKEN_DIR_ORG,
    TOKEN_DIR_STRINGZ,

    TOKEN_DIR_FILL,
} Token_Type;
static char *token_name[] = {
    [TOKEN_REG] = "TOKEN_REG",
    [TOKEN_ADD] = "TOKEN_ADD",
    [TOKEN_AND] = "TOKEN_AND",
    [TOKEN_NOT] = "TOKEN_NOT",
    [TOKEN_BR] = "TOKEN_BR",
    [TOKEN_LEA] = "TOKEN_LEA",
    [TOKEN_LD] = "TOKEN_LD",
    [TOKEN_LDI] = "TOKEN_LDI",
    [TOKEN_LDR] = "TOKEN_LDR",
    [TOKEN_ST] = "TOKEN_ST",
    [TOKEN_STI] = "TOKEN_STI",
    [TOKEN_STR] = "TOKEN_STR",
    [TOKEN_JMP] = "TOKEN_JMP",
    [TOKEN_JSR] = "TOKEN_JSR",
    [TOKEN_JSRR] = "TOKEN_JSRR",
    [TOKEN_RET] = "TOKEN_RET",
    [TOKEN_TRAP] = "TOKEN_TRAP",
    [TOKEN_RTI] = "TOKEN_RTI",
    [TOKEN_LABEL_DEF] = "TOKEN_LABEL_DEF",
    [TOKEN_LABEL_CALL] = "TOKEN_LABEL_CALL",
    [TOKEN_INT_LIT] = "TOKEN_INT_LIT",
    [TOKEN_ILLEGAL] = "TOKEN_ILLEGAL",
    [TOKEN_END] = "TOKEN_END",
    [TOKEN_DIR_ORG] = "TOKEN_DIR_ORG",
    [TOKEN_DIR_FILL] = "TOKEN_DIR_FILL",
};

typedef struct {
    Token_Type type;
    int operand;
    String_View content;
    Location loc;
} Token;

size_t word_count = 0;
size_t max_org_value = 0;

Token parse_next_token(Lexer *l, Label_Hashmap *lhm) {
    Token t = {0};
    t.type = TOKEN_END;
    for (int i = 0; l->cursor < l->size; i++) {
        String_Token st = lex_chop_token(l);
        t.content = st.content;
        if (st.content.len == 0)
            continue;
        t.loc = st.loc;

        if (st.content.data[0] == '%') {
            if (st.content.len == 0 || st.content.len > 3 ||
                st.content.data[1] != 'r' || st.content.data[2] - '0' > 7) {
                print_loc(st.loc);
                printf("[ERROR] invalid register `" SV_FMT "`\n", SV_ARG(st.content));
                t.type = TOKEN_ILLEGAL;
                exit(1);
            }
            st.content.data++;
            st.content.len--;
            t.type = TOKEN_REG;
            t.operand = st.content.data[1] - '0';
            return t;
        } else if (st.content.data[0] == '"') {
            t.type = TOKEN_STR_LIT;
            t.content = st.content;
            return t;
        } else if (st.content.data[0] == '.') {
            st.content.data++;
            st.content.len--;
            if (st.content.len <= 0) {
                print_loc(st.loc);
                printf("[ERROR] invalid directive `" SV_FMT "`\n", SV_ARG(st.content));
                exit(1);
            }
            if (sv_cmp(st.content, SV_STATIC("org"))) {
                t.type = TOKEN_DIR_ORG;
                return t;
            } else if (sv_cmp(st.content, SV_STATIC("fill"))) {
                t.type = TOKEN_DIR_FILL;
                return t;
            } else if (sv_cmp(st.content, SV_STATIC("stringz"))) {
                t.type = TOKEN_DIR_STRINGZ;
                return t;
            }
        } else if (st.content.data[0] == '$') {
            if (st.content.data[st.content.len - 1] == ':') {
                continue;
            }
            st.content.data++;
            st.content.len--;
            Label lbl = get_label(lhm, st.content);
            if (lbl.content.data == NULL) {
                print_loc(t.loc);
                printf("[ERROR] undefined label `" SV_FMT "`\n", SV_ARG(st.content));
                exit(1);
            }
            t.type = TOKEN_LABEL_CALL;
            t.operand = lbl.bytes_count;
            return t;
        } else if (st.content.data[0] == '#') {
            st.content.data++;
            st.content.len--;
            if (st.content.data[0] == 'x') {
                st.content.data++;
                st.content.len--;
                int number = 0;
                for (int i = 0; i < st.content.len; i++) {
                    if (isdigit(st.content.data[i])) {
                        number = number * 16 + (st.content.data[i] - '0');
                    } else if (st.content.data[i] >= 'a' && st.content.data[i] <= 'f') {
                        number = number * 16 + (st.content.data[i] - 'a' + 10);
                    } else {
                        printf("[ERROR] invalid hexadecimal literal`" SV_FMT "`\n",
                               SV_ARG(st.content));
                        exit(1);
                    }
                }
                t.operand = number;
                t.type = TOKEN_INT_LIT;
                return t;
            } else if (st.content.data[0] == 'b') {
                st.content.data++;
                st.content.len--;
                int number = 0;
                for (int i = 0; i < st.content.len; i++) {
                    switch (st.content.data[i]) {
                        case '0':
                            break;
                        case '1': {
                            number |= 1 << i;
                        } break;
                        default: {
                            printf("[ERROR] invalid binary literal`" SV_FMT "`\n",
                                   SV_ARG(st.content));
                            exit(1);
                        } break;
                    }
                }
                t.operand = number;
                t.type = TOKEN_INT_LIT;
                return t;
            }
            bool is_signed = false;
            if (st.content.data[0] == '-') {
                is_signed = true;
                st.content.data++;
                st.content.len--;
            }
            if (st.content.len == 0) {
                printf("[ERROR] invalid int literal`" SV_FMT "`\n", SV_ARG(st.content));
                printf("for hexadecimal literals use `#x`");
                printf("and for binary iterals use `#b`");
                exit(1);
            }
            int number = 0;
            for (int i = 0; i < st.content.len; i++) {
                if (!isdigit(st.content.data[i])) {
                    printf("[ERROR] invalid int literal`" SV_FMT "`\n",
                           SV_ARG(st.content));
                    exit(1);
                }
                number = number * 10 + (st.content.data[i] - '0');
            }

            t.type = TOKEN_INT_LIT;
            if (is_signed)
                t.operand = -number;
            else
                t.operand = number;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("add"))) {
            t.type = TOKEN_ADD;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("not"))) {
            t.type = TOKEN_NOT;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("and"))) {
            t.type = TOKEN_AND;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("jmp"))) {
            t.type = TOKEN_JMP;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("lea"))) {
            t.type = TOKEN_LEA;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("ld"))) {
            t.type = TOKEN_LD;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("ldi"))) {
            t.type = TOKEN_LDI;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("ldr"))) {
            t.type = TOKEN_LDR;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("st"))) {
            t.type = TOKEN_ST;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("sti"))) {
            t.type = TOKEN_STI;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("str"))) {
            t.type = TOKEN_STR;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("rti"))) {
            t.type = TOKEN_RTI;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("trap"))) {
            t.type = TOKEN_TRAP;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("jsr"))) {
            t.type = TOKEN_JSR;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("jsrr"))) {
            t.type = TOKEN_JSRR;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("ret"))) {
            t.type = TOKEN_RET;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("br"))) {
            t.type = TOKEN_BR;
            if (l->cursor + 1 >= l->size) {
                print_loc(st.loc);
                printf("[ERROR] invalid br instruction\n");
                t.type = TOKEN_ILLEGAL;
                return t;
            }
            String_Token args_tok = lex_chop_token(l);
            uint16_t operand = 0;
            if (sv_contains(args_tok.content, 'n')) {
                operand |= 0b100;
            }
            if (sv_contains(args_tok.content, 'z')) {
                operand |= 0b010;
            }
            if (sv_contains(args_tok.content, 'p')) {
                operand |= 0b001;
            }
            t.operand = operand;
            return t;
        } else {
            t.type = TOKEN_ILLEGAL;
            return t;
        }
    }
    return t;
}

void first_pass(Lexer *l, Label_Hashmap *lhm) {
    size_t current_org_val = 0;
    for (; l->cursor < l->size;) {
        String_Token st = lex_chop_token(l);
        if (st.content.len == 0)
            continue;
        if (st.content.data[0] == '$' &&
            st.content.data[st.content.len - 1] == ':') {
            if ((int)st.content.len - 3 <= 0) {
                print_loc(st.loc);
                printf("[ERROR] invalid label `" SV_FMT "`\n", SV_ARG(st.content));
            }
            st.content.data++;
            st.content.len -= 2;
            Label lbl = new_label(st.content, word_count + current_org_val);
            Label check = get_label(lhm, st.content);
            if (check.content.data != NULL) {
                print_loc(st.loc);
                printf("[ERROR] label redefined `" SV_FMT "`\n", SV_ARG(st.content));
                exit(1);
            }
            insert_label(lhm, st.content, lbl);
        } else if (sv_cmp(st.content, SV_STATIC(".org"))) {
            st = lex_chop_token(l);
            if (st.content.data[0] == '#') {
                st.content.data++;
                st.content.len--;
                if (st.content.data[0] == 'x') {
                    st.content.data++;
                    st.content.len--;
                    int number = 0;
                    for (int i = 0; i < st.content.len; i++) {
                        if (isdigit(st.content.data[i])) {
                            number = number * 16 + (st.content.data[i] - '0');
                        } else if (st.content.data[i] >= 'a' && st.content.data[i] <= 'f') {
                            number = number * 16 + (st.content.data[i] - 'a' + 10);
                        } else {
                            printf("[ERROR] invalid hexadecimal literal`" SV_FMT "`\n",
                                   SV_ARG(st.content));
                            exit(1);
                        }
                    }
                    current_org_val = word_count = number;
                }
            } else {
                continue;
            }
        } else if (sv_cmp(st.content, SV_STATIC("add"))) {
            word_count += 1;
        } else if (sv_cmp(st.content, SV_STATIC("not"))) {
            word_count += 1;
        } else if (sv_cmp(st.content, SV_STATIC("and"))) {
            word_count += 1;
        } else if (sv_cmp(st.content, SV_STATIC("jmp"))) {
            word_count += 1;
        } else if (sv_cmp(st.content, SV_STATIC("ld"))) {
            word_count += 1;
        } else if (sv_cmp(st.content, SV_STATIC("ldi"))) {
            word_count += 1;
        } else if (sv_cmp(st.content, SV_STATIC("ldr"))) {
            word_count += 1;
        } else if (sv_cmp(st.content, SV_STATIC("st"))) {
            word_count += 1;
        } else if (sv_cmp(st.content, SV_STATIC("sti"))) {
            word_count += 1;
        } else if (sv_cmp(st.content, SV_STATIC("str"))) {
            word_count += 1;
        } else if (sv_cmp(st.content, SV_STATIC("rti"))) {
            word_count += 1;
        } else if (sv_cmp(st.content, SV_STATIC("trap"))) {
            word_count += 1;
        } else if (sv_cmp(st.content, SV_STATIC("jsr"))) {
            word_count += 1;
        } else if (sv_cmp(st.content, SV_STATIC("jsrr"))) {
            word_count += 1;
        } else if (sv_cmp(st.content, SV_STATIC("ret"))) {
            word_count += 1;
        } else if (sv_cmp(st.content, SV_STATIC("br"))) {
            word_count += 1;
        } else if (sv_cmp(st.content, SV_STATIC(".fill"))) {
            word_count += 1;
        }
    }
}

int16_t get_label_pc_offset(uint16_t pc, uint16_t label_word) {
    int16_t diff = (label_word - pc * 2) - 1;
    return diff;
}

uint16_t compile_add(Token inst_token, Token dst, Token src1, Token src2) {
    if (dst.type != TOKEN_REG || src1.type != TOKEN_REG ||
        !(src2.type == TOKEN_REG || src2.type == TOKEN_INT_LIT)) {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for add instruction\n");
        printf("expected `add <dst_reg> <src_reg> <src_reg|imm_value(5)>`\n");
        exit(1);
    }
    uint16_t inst = 0;
    inst |= 0b0001 << 12;
    inst |= (dst.operand & 0b111) << 9;
    inst |= (src1.operand & 0b111) << 6;
    if (src2.type == TOKEN_REG) {
        inst |= (src2.operand & 0b111);
    } else if (src2.type == TOKEN_INT_LIT) {
        int16_t imm_val = src2.operand;
        if (imm_val < -8 || imm_val > 7) {
            print_loc(src2.loc);
            printf("[ERROR] immediate value for the `add` op should be in the range "
                   "(-8:7)\n");
            exit(1);
        }
        inst |= 1 << 5;
        inst |= (imm_val & 0b11111);
    } else {
        assert(false && "compile_add unreachable");
    }
    return inst;
}

uint16_t compile_and(Token inst_token, Token dst, Token src1, Token src2) {
    if (dst.type != TOKEN_REG || src1.type != TOKEN_REG ||
        !(src2.type == TOKEN_REG || src2.type == TOKEN_INT_LIT)) {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for and instruction\n");
        printf("expected `and <dst_reg> <src_reg> <src_reg|imm_value(5)>`\n");
        exit(1);
    }
    uint16_t inst = 0;
    inst |= 0b0101 << 12;
    inst |= (dst.operand & 0b111) << 9;
    inst |= (src1.operand & 0b111) << 6;
    if (src2.type == TOKEN_REG) {
        inst |= (src2.operand & 0b111);
    } else if (src2.type == TOKEN_INT_LIT) {
        int16_t imm_val = src2.operand;
        if (imm_val < -8 || imm_val > 7) {
            print_loc(src2.loc);
            printf("[ERROR] immediate value for the `and` op should be in the range "
                   "(-8:7)\n");
            exit(1);
        }
        inst |= 1 << 5;
        inst |= (imm_val & 0b11111);
    } else {
        assert(false && "compile_add unreachable");
    }
    return inst;
}

uint16_t compile_not(Token inst_token, Token dst_reg, Token src_reg) {
    uint16_t inst = 0;
    if (dst_reg.type != TOKEN_REG || src_reg.type != TOKEN_REG) {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `not` instruction\n");
        printf("expected `not <dst_reg> <src_reg>`\n");
        exit(1);
    }
    inst |= 0b1001 << 12;
    inst |= (dst_reg.operand & 0b111) << 9;
    inst |= (src_reg.operand & 0b111) << 6;
    inst |= (0b0000000000111111);
    return inst;
}

uint16_t compile_br(Token inst_token, Token offset_9) {
    if (!(offset_9.type == TOKEN_INT_LIT || offset_9.type == TOKEN_LABEL_CALL)) {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `br` instruction\n");
        printf("expected `br <n|z|p> <pc_offset>`\n");
        printf("example: `br nzp #3`\n");
        exit(1);
    }
    uint16_t inst = 0;
    inst |= (inst_token.operand & 0b111) << 9;

    if (offset_9.type == TOKEN_INT_LIT) {
        inst |= (offset_9.operand & 0b111111111) << 0;
    } else if (offset_9.type == TOKEN_LABEL_CALL) {
        int rel_addr = get_label_pc_offset(word_count, offset_9.operand);
        inst |= (rel_addr & 0b111111111) << 0;
    } else {
        assert(false && "compile_br unreachable");
    }

    return inst;
}

uint16_t compile_jmp(Token inst_token, Token base_reg) {
    if (base_reg.type != TOKEN_REG) {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `jmp` instruction\n");
        printf("expected `jmp <base_reg>`\n");
        exit(1);
    }
    uint16_t inst = 0;
    inst |= 0b1100 << 12;
    inst |= (base_reg.operand & 0b111) << 6;
    return inst;
}

uint16_t compile_ret(Token inst_token) {
    uint16_t inst = 0;
    inst |= 0b1100 << 12;
    inst |= (0b111 & 7) << 6;
    return inst;
}

uint16_t compile_ld(Token inst_token, Token dst_reg, Token offset_9) {
    if (!(dst_reg.type == TOKEN_REG || offset_9.type == TOKEN_INT_LIT ||
        offset_9.type == TOKEN_LABEL_CALL)) {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `ld` instruction\n");
        printf("expected `ld <dst_reg> <pc_offset(9)>`\n");
        exit(1);
    }
    uint16_t inst = 0;
    inst |= 0b0010 << 12;
    inst |= ((dst_reg.operand) & 0b111) << 9;
    if (offset_9.type == TOKEN_INT_LIT) {
        inst |= (offset_9.operand & 0b111111111);
        return inst;
    } else if (offset_9.type == TOKEN_LABEL_CALL) {
        int16_t offset = get_label_pc_offset(word_count, offset_9.operand);
        inst |= (offset & 0b111111111);
        return inst;
    } else {
        assert(false && "unreachable");
    }
}
uint16_t compile_ldi(Token inst_token, Token dst_reg, Token offset_9) {
    if (dst_reg.type != TOKEN_REG || offset_9.type != TOKEN_INT_LIT) {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `ldi` instruction\n");
        printf("expected `ldi <dst_reg> <pc_offset(9)>`\n");
        exit(1);
    }
    uint16_t inst = 0;
    inst |= 0b1010 << 12;
    inst |= (dst_reg.operand & 0b111) << 9;
    inst |= (offset_9.operand & 0b111111111);
    return inst;
}

uint16_t compile_ldr(Token inst_token, Token dst_reg, Token base_reg,
                     Token offset_9) {
    if (dst_reg.type != TOKEN_REG || base_reg.type != TOKEN_REG ||
        offset_9.type != TOKEN_INT_LIT) {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `ldr` instruction\n");
        printf("expected `ldr <dst_reg> <base_reg> <pc_offset(6)>`\n");
        exit(1);
    }
    uint16_t inst = 0;
    inst |= 0b0110 << 12;
    inst |= (dst_reg.operand & 0b111) << 9;
    inst |= (base_reg.operand & 0b111) << 6;
    inst |= (offset_9.operand & 0b111111);
    return inst;
}

uint16_t compile_st(Token inst_token, Token src_reg, Token offset_9) {
    if (src_reg.type != TOKEN_REG || offset_9.type != TOKEN_INT_LIT) {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `st` instruction\n");
        printf("expected `st <src_reg> <pc_offset(9)>`\n");
        exit(1);
    }
    uint16_t inst = 0;
    inst |= 0b0011 << 12;
    inst |= (src_reg.operand & 0b111) << 9;
    inst |= (offset_9.operand & 0b111111111);
    return inst;
}

uint16_t compile_sti(Token inst_token, Token src_reg, Token offset_9) {
    if (src_reg.type != TOKEN_REG || offset_9.type != TOKEN_INT_LIT) {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `sti` instruction\n");
        printf("expected `sti <src_reg> <pc_offset(9)>`\n");
        exit(1);
    }
    uint16_t inst = 0;
    inst |= 0b1011 << 12;
    inst |= (src_reg.operand & 0b111) << 9;
    inst |= (offset_9.operand & 0b111111111);
    return inst;
}

uint16_t compile_str(Token inst_token, Token src_reg, Token base_reg,
                     Token offset_9) {
    if (src_reg.type != TOKEN_REG || base_reg.type != TOKEN_REG ||
        offset_9.type != TOKEN_INT_LIT) {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `str` instruction\n");
        printf("expected `str <src_reg> <base_reg> <pc_offset(9)>`\n");
        exit(1);
    }
    uint16_t inst = 0;
    inst |= 0b0111 << 12;
    inst |= (src_reg.operand & 0b111) << 9;
    inst |= (base_reg.operand & 0b111) << 6;
    inst |= (offset_9.operand & 0b111111);
    return inst;
}

uint16_t compile_rti() {
    uint16_t inst = 0;
    inst |= 0b1000 << 12;
    return inst;
}

uint16_t compile_trap(Token inst_token, Token trapvect_8) {
    if (trapvect_8.type != TOKEN_INT_LIT) {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `trap` instruction\n");
        printf("expected `trap <trap_vector(8)>`\n");
        exit(1);
    }
    if (trapvect_8.operand < 0 || trapvect_8.operand > 255) {
        printf("[ERROR] invalid operands to for `trap` instruction\n");
        printf("operand `trap` op should be in the range (0:255)\n");
        exit(1);
    }
    uint16_t inst = 0;
    inst |= 0b1111 << 12;
    inst |= trapvect_8.operand & 0b11111111;
    return inst;
}

uint16_t compile_lea(Token inst_token, Token dst_reg, Token offset_9) {
    if (dst_reg.type != TOKEN_REG || 
        !(offset_9.type == TOKEN_INT_LIT || offset_9.type == TOKEN_LABEL_CALL)) 
    {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `lea` instruction\n");
        printf("expected `lea <dst_reg> <pc_offset(9)>`\n");
        exit(1);
    }
    uint16_t inst = 0;
    inst |= 0b1110 << 12;
    inst |= (dst_reg.operand & 0b111) << 9;
    if (offset_9.type == TOKEN_INT_LIT) {
        inst |= (offset_9.operand & 0b111111111);
        return inst;
    } else if (offset_9.type == TOKEN_LABEL_CALL) {
        int16_t offset = get_label_pc_offset(word_count, offset_9.operand) + 1;
        inst |= (offset & 0b111111111);
        return inst;
    } else {
        assert(false && "unreachable");
    }
}

uint16_t compile_jsr(Token inst_token, Token base_or_offset_11) {
    if (!(base_or_offset_11.type == TOKEN_INT_LIT || base_or_offset_11.type == TOKEN_LABEL_CALL)) {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `jsr` instruction\n");
        printf("expected `jsr <dst_reg> <pc_offset(9)>`\n");
        exit(1);
    }
    uint16_t inst = 0;
    inst |= 0b0100 << 12;
    inst |= 1 << 11;
    if (base_or_offset_11.type == TOKEN_INT_LIT) {
        inst |= (base_or_offset_11.operand & 0b11111111111);
    } else if (base_or_offset_11.type == TOKEN_LABEL_CALL) {
        int16_t offset = get_label_pc_offset(word_count, base_or_offset_11.operand);
        inst |= offset & 0b11111111111;
        return inst;
    } else {
        assert(false && "compile_jsr unreachable");
    }
    return inst;
}

uint16_t compile_jsrr(Token inst_token, Token src_reg) {
    if (src_reg.type != TOKEN_REG) {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `jsr` instruction\n");
        printf("expected `jsr <dst_reg> <pc_offset(9)>`\n");
        exit(1);
    }
    printf("src_reg %u\n", src_reg.operand);
    uint16_t inst = 0;
    inst |= 0b0100 << 12;
    inst |= (src_reg.operand & 0b111) << 6;
    return inst;
}

int main() {
    Label_Hashmap lhm = label_hminit(100);

    char *file_path = "test.asm";
    size_t size = 0;
    char *content = read_file(file_path, &size);
    if (!content) {
        printf("[ERROR] could not read file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }
    char *out_path = "test.bin";
    FILE *out_file = fopen(out_path, "wb");
    if (!out_file) {
        printf("[ERROR] could not open out file `%s`: %s\n", out_path,
               strerror(errno));
        exit(1);
    }

    Lexer first_pass_l = lex_new(content, size, file_path);
    first_pass(&first_pass_l, &lhm);
    Lexer l = lex_new(content, size, file_path);
    Token t = {0};
    word_count = 0;

    for (; l.cursor < l.size;) {
        t = parse_next_token(&l, &lhm);
        switch (t.type) {
            case TOKEN_ADD: {
                Token dst_reg = parse_next_token(&l, &lhm);
                Token src_value1 = parse_next_token(&l, &lhm);
                Token src_value2 = parse_next_token(&l, &lhm);
                uint16_t inst = compile_add(t, dst_reg, src_value1, src_value2);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_AND: {
                Token dst_reg = parse_next_token(&l, &lhm);
                Token src_value1 = parse_next_token(&l, &lhm);
                Token src_value2 = parse_next_token(&l, &lhm);
                uint16_t inst = compile_and(t, dst_reg, src_value1, src_value2);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_NOT: {
                Token dst_reg = parse_next_token(&l, &lhm);
                Token src_reg = parse_next_token(&l, &lhm);
                uint16_t inst = compile_not(t, dst_reg, src_reg);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_BR: {
                Token offset = parse_next_token(&l, &lhm);
                uint16_t inst = compile_br(t, offset);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_JMP: {
                Token base_reg = parse_next_token(&l, &lhm);
                uint16_t inst = compile_jmp(t, base_reg);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_LD: {
                Token dst_reg = parse_next_token(&l, &lhm);
                Token offset_9 = parse_next_token(&l, &lhm);
                uint16_t inst = compile_ld(t, dst_reg, offset_9);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_LDI: {
                Token dst_reg = parse_next_token(&l, &lhm);
                Token offset_9 = parse_next_token(&l, &lhm);
                uint16_t inst = compile_ldi(t, dst_reg, offset_9);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_LDR: {
                Token dst_reg = parse_next_token(&l, &lhm);
                Token base_reg = parse_next_token(&l, &lhm);
                Token offset_9 = parse_next_token(&l, &lhm);
                uint16_t inst = compile_ldr(t, dst_reg, base_reg, offset_9);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_ST: {
                Token src_reg = parse_next_token(&l, &lhm);
                Token offset_9 = parse_next_token(&l, &lhm);
                uint16_t inst = compile_st(t, src_reg, offset_9);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_STI: {
                Token src_reg = parse_next_token(&l, &lhm);
                Token offset_9 = parse_next_token(&l, &lhm);
                uint16_t inst = compile_sti(t, src_reg, offset_9);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_STR: {
                Token src_reg = parse_next_token(&l, &lhm);
                Token base_reg = parse_next_token(&l, &lhm);
                Token offset_9 = parse_next_token(&l, &lhm);
                uint16_t inst = compile_str(t, src_reg, base_reg, offset_9);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_RTI: {
                uint16_t inst = compile_rti();
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_TRAP: {
                Token trapvec_8 = parse_next_token(&l, &lhm);
                uint16_t inst = compile_trap(t, trapvec_8);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_LEA: {
                Token dst_reg = parse_next_token(&l, &lhm);
                Token offset_9 = parse_next_token(&l, &lhm);
                uint16_t inst = compile_lea(t, dst_reg, offset_9);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_JSR: {
                Token offset_9 = parse_next_token(&l, &lhm);
                uint16_t inst = compile_jsr(t, offset_9);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_JSRR: {
                Token src_reg = parse_next_token(&l, &lhm);
                uint16_t inst = compile_jsrr(t, src_reg);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_RET: {
                uint16_t inst = compile_ret(t);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_DIR_FILL: {
                Token fill_word = parse_next_token(&l, &lhm);
                if (!(fill_word.type == TOKEN_INT_LIT ||
                    fill_word.type == TOKEN_LABEL_CALL)) {
                    print_loc(fill_word.loc);
                    printf("[ERROR] expected int literal found `" SV_FMT "`\n",
                           SV_ARG(fill_word.content));
                    exit(1);
                }
                if (fill_word.type == TOKEN_INT_LIT) {
                    if ((uint)fill_word.operand > MAX_UINT16_T) {
                        print_loc(fill_word.loc);
                        printf("[ERROR] expected int literal of size less than `%d`\n",
                               MAX_UINT16_T);
                        exit(1);
                    } 
                    fwrite(&fill_word.operand, 2, 1, out_file);
                } else if (fill_word.type == TOKEN_LABEL_CALL) {
                    uint16_t addr = fill_word.operand;
                    fwrite(&addr, 2, 1, out_file);
                } else {
                    assert(false && "unreachable");
                }
                word_count += 1;
            } break;
            case TOKEN_DIR_ORG: {
                Token org_amount = parse_next_token(&l, &lhm);
                if (org_amount.type != TOKEN_INT_LIT || org_amount.operand < 0) {
                    print_loc(org_amount.loc);
                    printf("[ERROR] expected int literal found `" SV_FMT "`\n",
                           SV_ARG(org_amount.content));
                    exit(1);
                }
                size_t diff_in_bytes = org_amount.operand - word_count;
                uint16_t zero_word = 0;
                for (int i = 0; i < diff_in_bytes; i++) {
                    fwrite(&zero_word, 2, 1, out_file);
                }
                max_org_value = word_count = org_amount.operand;
            } break;
            case TOKEN_DIR_STRINGZ: {
                Token string = parse_next_token(&l, &lhm);
                if (string.type != TOKEN_STR_LIT) {
                    print_loc(t.loc);
                    printf("[ERROR] expected string literal\n");
                    exit(1);
                }
                string.content.data++;
                string.content.len -= 2;
                for (int i = 0; i < string.content.len; i++) {
                    uint16_t word = (uint16_t)string.content.data[i];
                    fwrite(&word, 2, 1, out_file);
                }
            } break;
            case TOKEN_ILLEGAL: {
                print_loc(t.loc);
                printf("[ERROR] illegal token `" SV_FMT "`\n", SV_ARG(t.content));
                exit(1);
            } break;
            default: {
            } break;
        }
        if (t.type < TOKEN_COUNT) {
            word_count += 1;
        }
    }
}
