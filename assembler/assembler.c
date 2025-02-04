#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    char* data;
    size_t len;
} String_View;

#define SV_STATIC(str) (String_View) { .data = str, .len = sizeof(str) - 1 }

bool sv_cmp(const String_View sv1, const String_View sv2) {
    if (sv1.len != sv2.len) return false;
    for (int i = 0; i < sv1.len; i++) 
        if (sv1.data[i] != sv2.data[i]) return false;
    return true;
}

#define SV_FMT   "%.*s"
#define SV_ARG(SV) ((int)(SV).len), (SV).data

char* read_file(const char* file_name, size_t* size) {
    FILE* file;
    file = fopen(file_name, "r");

    if (file == NULL) return NULL;

    fseek(file, 0, SEEK_END);
    int len = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* content = malloc(sizeof(char)*len);
    for (int i = 0; i < len; i++) {
        content[i] = fgetc(file);
    }
    if (size) *size = len;
    return content;
}

typedef struct {
    char* content;
    size_t size;
    size_t cursor;
    size_t lineNo;
    size_t bol;
    char* file_path;
} Lexer;

typedef struct {
    size_t lineNo;
    size_t colNo;
    char* file_path;
} Location;

void print_loc(const Location l) {
    printf("%s:%zu:%zu ", l.file_path, l.lineNo, l.colNo);
}

typedef struct {
    String_View content;
    Location loc;
} String_Token;

Lexer lex_new(char* content, size_t size, char* file_path) {
    return (Lexer) {
        .content = content,
        .size = size,
        .file_path = file_path,
    };
}

void lex_skip_space(Lexer* l) {
    for (;isspace(l->content[l->cursor]) && 
            l->cursor < l->size; l->cursor++) 
    {
        if (l->content[l->cursor] == '\n') {
            l->lineNo++;
            l->bol = l->cursor+1;
        }
    }
}

String_Token lex_chop_token(Lexer* l) {
    String_Token st = {0};
    st.loc.file_path = l->file_path;
    String_View sv = {0};
    lex_skip_space(l);
    sv.data = &l->content[l->cursor];
    sv.len = 0;
    for (int i = 0; l->cursor < l->size; i++) {
        if (isspace(l->content[l->cursor])) {
            st.content = sv;
            st.loc.lineNo = l->lineNo;
            st.loc.colNo = i - l->bol;
            return st;
        }
        l->cursor++;
        sv.len++;
    }
    return st;
}

typedef enum {
    TOKEN_ILLEGAL,
    TOKEN_REG,
    TOKEN_ADD,
    TOKEN_AND,
    TOKEN_INT_LIT,
    TOKEN_END,
} Token_Type;

static char* token_str[] = {
    [TOKEN_REG] = "TOKEN_REG",
    [TOKEN_ADD] = "TOKEN_ADD",
    [TOKEN_INT_LIT] = "TOKEN_INT_LIT",
    [TOKEN_ILLEGAL] = "TOKEN_ILLEGAL",
    [TOKEN_END] = "TOKEN_END",
};

typedef struct {
    Token_Type type;
    int operand;
    Location loc;
} Token;

Token parse_next_token(Lexer* l) {
    Token t = {0};
    t.type = TOKEN_END;
    for (int i = 0; l->cursor < l->size; i++) {
        String_Token st = lex_chop_token(l);
        if (st.content.len == 0) continue;
        t.loc = st.loc;

        if (st.content.data[0] == '%') {
            st.content.data++;
            st.content.len--;
            if (st.content.len == 0 || st.content.len > 2 || st.content.data[0] != 'r' || st.content.data[1] - '0' > 7) {
                printf("[ERROR] invalid register `"SV_FMT"`\n", SV_ARG(st.content));
                t.type = TOKEN_ILLEGAL;
                return t;
            }
            t.type = TOKEN_REG;
            t.operand = st.content.data[1] - '0';
            return t;
        } else if (st.content.data[0] == '#') {
            st.content.data++;
            st.content.len--;
            if (st.content.len == 0) {
                printf("[ERROR] invalid int literal`"SV_FMT"`\n", SV_ARG(st.content));
                t.type = TOKEN_ILLEGAL;
                return t;
            }
            int number = 0;
            for (int i = 0; i < st.content.len; i++) {
                if (!isdigit(st.content.data[i])) {
                    printf("[ERROR] invalid int literal`"SV_FMT"`\n", SV_ARG(st.content));
                    t.type = TOKEN_ILLEGAL;
                    return t;
                }
                number = number * 10 + (st.content.data[i] - '0');
            }
            t.type = TOKEN_INT_LIT;
            t.operand = number;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("add"))) {
            t.type = TOKEN_ADD;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("and"))) {
            t.type = TOKEN_AND;
            return t;
        } else {
            t.type = TOKEN_ILLEGAL;
            return t;
        }
    }
    return t;
}

// Op_BR   = 0,
// Op_ADD  = 1,
// Op_LD   = 2,
// Op_ST   = 3,
// Op_JSR  = 4,
// Op_AND  = 5,
// Op_LDR  = 6,
// Op_STR  = 7,
// Op_RTI  = 8,
// Op_NOT  = 9,
// Op_LDI  = 10,
// Op_STI  = 11,
// Op_JMP  = 12,
// Op_RES  = 13,
// Op_LEA  = 14,
// Op_TRAP = 15,

uint16_t compile_add(Token inst_token, Token dst, Token src1, Token src2) {
    if (dst.type != TOKEN_REG || src1.type != TOKEN_REG ||
        !(src2.type == TOKEN_REG || src2.type == TOKEN_INT_LIT)) 
    {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for add instruction\n");
        printf("expected `add <dst_reg> <src_reg> <src_reg|imm_value(5)>`\n");
        exit(1);
    }
    uint16_t inst = 0;
    inst |= 0b0001                 << 12;
    inst |= (dst.operand & 0b111)  << 9;
    inst |= (src1.operand & 0b111) << 6;
    if (src2.type == TOKEN_REG) {
        inst |= (src2.operand & 0b111);
    } else if (src2.type == TOKEN_INT_LIT) {
        int16_t imm_val = src2.operand & 0b11111;
        if (imm_val < -8 || imm_val > 7) {
            print_loc(src2.loc);
            printf("[ERROR] immediate value for the `add` op should be in the range (-8:7)\n");
            exit(1);
        }
        inst |= 1 << 5;
        inst |= imm_val;
    } else {
        assert(false && "unreachable");
    }
    return inst;
}

uint16_t compile_and(Token inst_token, Token dst, Token src1, Token src2) {
    if (dst.type != TOKEN_REG || src1.type != TOKEN_REG ||
        !(src2.type == TOKEN_REG || src2.type == TOKEN_INT_LIT)) 
    {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for and instruction\n");
        printf("expected `and <dst_reg> <src_reg> <src_reg|imm_value(5)>`\n");
        exit(1);
    }
    uint16_t inst = 0;
    inst |= 0b0101 << 12;
    inst |= (dst.operand & 0b111)  << 9;
    inst |= (src1.operand & 0b111) << 6;
    if (src2.type == TOKEN_REG) {
        inst |= (src2.operand & 0b111);
    } else if (src2.type == TOKEN_INT_LIT) {
        int16_t imm_val = src2.operand & 0b11111;
        printf("immv %i\n", imm_val);
        if (imm_val < -8 || imm_val > 7) {
            print_loc(src2.loc);
            printf("[ERROR] immediate value for the `add` op should be in the range (-8:7)\n");
            exit(1);
        }
        inst |= 1 << 5;
        inst |= imm_val;
        printf("0x%x\n", inst);
    } else {
        assert(false && "unreachable");
    }
    return inst;
}

int main() {
    char* file_path = "test.asm";
    size_t size = 0;
    char* content = read_file(file_path, &size);
    if (!content) {
        printf("[ERROR] could not read file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }
    char* out_path = "test.bin";
    FILE* out_file = fopen(out_path, "wb");
    if (!out_file) {
        printf("[ERROR] could not open out file `%s`: %s\n", out_path, strerror(errno));
        exit(1);
    }
    Lexer l = lex_new(content, size, file_path);
    Token t = {0};
    for (;l.cursor < l.size;) {
        t = parse_next_token(&l);
        switch (t.type) {
            case TOKEN_ADD: {
                Token dst_reg = parse_next_token(&l);
                Token src_value1 = parse_next_token(&l);
                Token src_value2 = parse_next_token(&l);
                uint16_t inst = compile_add(t, dst_reg, src_value1, src_value2);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_AND: {
                Token dst_reg = parse_next_token(&l);
                Token src_value1 = parse_next_token(&l);
                Token src_value2 = parse_next_token(&l);
                uint16_t inst = compile_and(t, dst_reg, src_value1, src_value2);
                printf("inst: 0x%x\n", inst);
                fwrite(&inst, 2, 1, out_file);
            } break;
            default: {
            } break;
        }
    }
}

