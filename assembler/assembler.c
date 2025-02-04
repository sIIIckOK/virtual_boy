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

bool sv_contains(const String_View sv, const char ch) {
    for (int i = 0; i < sv.len; i++) {
        if (sv.data[i] == ch) return true; 
    }
    return false;
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
        .lineNo = 1,
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
            l->bol = l->cursor;
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
    TOKEN_END,
    TOKEN_ILLEGAL,
    TOKEN_REG,
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
    TOKEN_RET,
    TOKEN_BR,
    TOKEN_INT_LIT,
} Token_Type;

static char* token_name[] = {
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
    [TOKEN_RET] = "TOKEN_RET",
    [TOKEN_RTI] = "TOKEN_RTI",
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
            bool is_signed = false;
            if (st.content.data[0] == '-') {
                is_signed = true;
                st.content.data++;
                st.content.len--;
            }
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
            if (is_signed) t.operand = -number;
            else           t.operand = number;
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
        } else if (sv_cmp(st.content, SV_STATIC("ret"))) {
            t.type = TOKEN_RET;
            return t;
        } else if (sv_cmp(st.content, SV_STATIC("br"))) {
            t.type = TOKEN_BR;
            if (l->cursor + 1 >= l->size) {
                printf("[ERROR] invalid int literal`"SV_FMT"`\n", SV_ARG(st.content));
                t.type = TOKEN_ILLEGAL;
                return t;
            }
            String_Token args_tok = lex_chop_token(l);
            uint16_t operand = 0;
            if (sv_contains(args_tok.content, 'n')) {
                operand |= 0b100;
            } else if (sv_contains(args_tok.content, 'z')) { 
                operand |= 0b010;
            } else if (sv_contains(args_tok.content, 'p')) {
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
        int16_t imm_val = src2.operand;
        if (imm_val < -8 || imm_val > 7) {
            print_loc(src2.loc);
            printf("[ERROR] immediate value for the `add` op should be in the range (-8:7)\n");
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
    if (offset_9.type != TOKEN_INT_LIT) {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `br` instruction\n");
        printf("expected `br <n|z|p> <pc_offset>`\n");
        printf("example: `br nzp #3`\n");
        exit(1);
    }
    uint16_t inst = 0;
    inst |= (inst_token.operand & 0b111)   << 9;
    inst |= (offset_9.operand & 0b111111111) << 0;
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
    inst |= base_reg.operand & 0b111 << 6;
    return inst;
}

uint16_t compile_ld(Token inst_token, Token dst_reg, Token offset_9) {
    if (dst_reg.type != TOKEN_REG || offset_9.type != TOKEN_INT_LIT) {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `ld` instruction\n");
        printf("expected `ld <dst_reg> <pc_offset(9)>`\n");
        exit(1);
    }
    uint16_t inst;
    inst |= 0b0010 << 12;
    inst |= (dst_reg.operand & 0b111) << 9;
    inst |= (offset_9.operand & 0b111111111);
    return inst;
}
uint16_t compile_ldi(Token inst_token, Token dst_reg, Token offset_9) {
    if (dst_reg.type != TOKEN_REG || offset_9.type != TOKEN_INT_LIT) {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `ldi` instruction\n");
        printf("expected `ldi <dst_reg> <pc_offset(9)>`\n");
        exit(1);
    }
    uint16_t inst;
    inst |= 0b1010 << 12;
    inst |= (dst_reg.operand & 0b111) << 9;
    inst |= (offset_9.operand & 0b111111111);
    return inst;
}

uint16_t compile_ldr(Token inst_token, Token dst_reg, Token base_reg, Token offset_9) {
    if (dst_reg.type != TOKEN_REG || base_reg.type != TOKEN_REG 
        ||offset_9.type != TOKEN_INT_LIT) 
    {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `ldr` instruction\n");
        printf("expected `ldr <dst_reg> <base_reg> <pc_offset(6)>`\n");
        exit(1);
    }
    uint16_t inst;
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
    uint16_t inst;
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
    uint16_t inst;
    inst |= 0b1011 << 12;
    inst |= (src_reg.operand & 0b111) << 9;
    inst |= (offset_9.operand & 0b111111111);
    return inst;
}

uint16_t compile_str(Token inst_token, Token src_reg, Token base_reg, Token offset_9) {
    if (src_reg.type != TOKEN_REG || base_reg.type != TOKEN_REG 
        ||offset_9.type != TOKEN_INT_LIT) 
    {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `str` instruction\n");
        printf("expected `str <src_reg> <base_reg> <pc_offset(9)>`\n");
        exit(1);
    }
    uint16_t inst;
    inst |= 0b0111 << 12;
    inst |= (src_reg.operand & 0b111) << 9;
    inst |= (base_reg.operand & 0b111) << 6;
    inst |= (offset_9.operand & 0b111111);
    return inst;
}

uint16_t compile_rti() {
    uint16_t inst;
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
    uint16_t inst;
    inst |= 0b1111 << 12;
    inst |= trapvect_8.operand & 0b11111111;
    return inst;
}

uint16_t compile_lea(Token inst_token, Token dst_reg, Token offset_9) {
    if (dst_reg.type != TOKEN_REG || offset_9.type != TOKEN_INT_LIT) {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `lea` instruction\n");
        printf("expected `lea <dst_reg> <pc_offset(9)>`\n");
        exit(1);
    }
    uint16_t inst;
    inst |= 0b1110 << 12;
    inst |= (dst_reg.operand & 0b111) << 9;
    inst |= (offset_9.operand & 0b111111111);
    return inst;
}

uint16_t compile_jsr(Token inst_token, Token base_or_offset_11) {
    if (base_or_offset_11.type != TOKEN_REG || base_or_offset_11.type != TOKEN_INT_LIT) 
    {
        print_loc(inst_token.loc);
        printf("[ERROR] invalid operands to for `jsr` instruction\n");
        printf("expected `jsr <dst_reg> <pc_offset(9)>`\n");
        exit(1);
    }
    uint16_t inst;
    inst |= 0b0100 << 12;
    if (base_or_offset_11.type == TOKEN_INT_LIT) {
        inst |= 1 << 11;
        inst |= (base_or_offset_11.operand & 0b11111111111);
    } else if (base_or_offset_11.type == TOKEN_REG) {
        inst |= (base_or_offset_11.operand & 0b111) << 6;
    } else {
        assert(false && "compile_jsr unreachable");
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
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_NOT: {
                Token dst_reg = parse_next_token(&l);
                Token src_reg = parse_next_token(&l);
                uint16_t inst = compile_not(t, dst_reg, src_reg);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_BR: {
                Token offset = parse_next_token(&l);
                uint16_t inst = compile_br(t, offset);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_JMP: {
                Token base_reg = parse_next_token(&l);
                uint16_t inst = compile_jmp(t, base_reg);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_LD: {
                Token dst_reg = parse_next_token(&l);
                Token offset_9 = parse_next_token(&l);
                uint16_t inst = compile_ld(t, dst_reg, offset_9);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_LDI: {
                Token dst_reg = parse_next_token(&l);
                Token offset_9 = parse_next_token(&l);
                uint16_t inst = compile_ldi(t, dst_reg, offset_9);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_LDR: {
                Token dst_reg = parse_next_token(&l);
                Token base_reg = parse_next_token(&l);
                Token offset_9 = parse_next_token(&l);
                uint16_t inst = compile_ldr(t, dst_reg, base_reg, offset_9);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_ST: {
                Token src_reg = parse_next_token(&l);
                Token offset_9 = parse_next_token(&l);
                uint16_t inst = compile_st(t, src_reg, offset_9);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_STI: {
                Token src_reg = parse_next_token(&l);
                Token offset_9 = parse_next_token(&l);
                uint16_t inst = compile_sti(t, src_reg, offset_9);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_STR: {
                Token src_reg = parse_next_token(&l);
                Token base_reg = parse_next_token(&l);
                Token offset_9 = parse_next_token(&l);
                uint16_t inst = compile_str(t, src_reg, base_reg, offset_9);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_RTI: {
                uint16_t inst = compile_rti();
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_TRAP: {
                Token trapvec_8 = parse_next_token(&l);
                uint16_t inst = compile_trap(t, trapvec_8);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_LEA: {
                Token dst_reg = parse_next_token(&l);
                Token offset_9 = parse_next_token(&l);
                uint16_t inst = compile_lea(t, dst_reg, offset_9);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_JSR: {
                Token dst_reg = parse_next_token(&l);
                Token offset_9 = parse_next_token(&l);
                uint16_t inst = compile_lea(t, dst_reg, offset_9);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_RET: {
                Token base_reg = {
                    .type = TOKEN_REG,
                    .operand = 7,
                };
                uint16_t inst = compile_jmp(t, base_reg);
                fwrite(&inst, 2, 1, out_file);
            } break;
            case TOKEN_ILLEGAL: {
                print_loc(t.loc);
                printf("[ERROR] illegal token\n");
                exit(1);
            } break;
            default: {
            } break;
        }
    }
}

