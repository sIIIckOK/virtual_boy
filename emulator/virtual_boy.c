#include <bits/pthreadtypes.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef uint16_t uWord;
typedef int16_t   Word;

#define MEMORY_SIZE 65535

typedef uint16_t Memory[MEMORY_SIZE]; // [ALERT] cant use signed char instead of uint8 
//                                      // as signed char get sign extended by C which messes with the data
//                                      // could also use `unsigned char`

static bool program_should_close = false;
 
// memory layout:
// Trap Vector Table      : 0x0000 - 0x00FF
// Interrupt Vector Table : 0x0100 - 0x01FF
// OS Space               : 0x0200 - 0x2FFF
// User Space             : 0x3000 - 0xFDFF
// I/O Register Space     : 0xFE00 - 0xFFFF

enum Mem_Landmark {
    MEM_BEGIN          = 0x0000,
    MEM_END            = 0xFFFF,

    MEM_TRAPVT_BEGIN   = 0x0000,
    MEM_TRAPVT_END     = 0x00FF,

    MEM_INTERVT_BEGIN  = 0x0100,
    MEM_INTERVT_END    = 0x01FF,

    MEM_OSSPC_BEGIN    = 0x0200,
    MEM_OSSPC_END      = 0x2FFF,

    MEM_USERSPC_BEGIN  = 0x3000,
    MEM_USERSPC_END    = 0xFDFF,

    MEM_IOREG_BEGIN    = 0xFE00,
    MEM_IOREG_END      = 0xFFFF,
};

typedef uWord Instruction;

#define PSR_BIT_SSM (1 << 15)

#define VEC_PRIV_MODE_VIOLATION (0x00)

typedef enum {
    Op_BR   = 0,
    Op_ADD  = 1,
    Op_LD   = 2,
    Op_ST   = 3,
    Op_JSR  = 4,
    Op_AND  = 5,
    Op_LDR  = 6,
    Op_STR  = 7,
    Op_RTI  = 8,
    Op_NOT  = 9,
    Op_LDI  = 10,
    Op_STI  = 11,
    Op_JMP  = 12,
    Op_RES  = 13,
    Op_LEA  = 14,
    Op_TRAP = 15,
} Op_Id;

static char* op_name[] = {
    [Op_BR] = "Op_BR",
    [Op_ADD] = "Op_ADD",
    [Op_LD] = "Op_LD",
    [Op_ST] = "Op_ST",
    [Op_JSR] = "Op_JSR",
    [Op_AND] = "Op_AND",
    [Op_LDR] = "Op_LDR",
    [Op_STR] = "Op_STR",
    [Op_RTI] = "Op_RTI",
    [Op_NOT] = "Op_NOT",
    [Op_LDI] = "Op_LDI",
    [Op_STI] = "Op_STI",
    [Op_JMP] = "Op_JMP",
    [Op_RES] = "Op_RES",
    [Op_LEA] = "Op_LEA",
    [Op_TRAP] = "Op_TRAP",
};

typedef struct {
    Word  registers[8];
    uWord PC;
    uWord IR;
    uWord PSR;
} Machine;

typedef struct {
    uint8_t* bytes;
    size_t capacity; 
    size_t count; 
} Byte_Data;

Byte_Data init_byte_data_size(size_t size) {
    Byte_Data byte_data = {0};
    byte_data.count = 0;
    byte_data.capacity = size;
    byte_data.bytes = malloc(sizeof(*byte_data.bytes)*byte_data.capacity);
    return byte_data;
}

Byte_Data init_byte_data() {
    Byte_Data byte_data = {0};
    byte_data.count = 0;
    byte_data.capacity = 5;
    byte_data.bytes = (uint8_t*)malloc(sizeof(uWord)*byte_data.capacity);
    return byte_data;
}

void push_data(Byte_Data* byte_data, uWord data) {
    if (byte_data->count >= byte_data->capacity) {
        byte_data->capacity = (byte_data->capacity + 1) * 2;
        byte_data->bytes = realloc(byte_data->bytes, sizeof(uint8_t) * byte_data->capacity);
    }
    byte_data->bytes[byte_data->count] = data;
    byte_data->count++;
}

Word sext(int val, size_t size) {
    int sign_bit = (val << (sizeof(val)*8 - size - 1)) >> (sizeof(val)*8 - 2);
    Word mask = (1 << size) - 1;
    Word res = val & mask;
    if (sign_bit) res |= ~mask;

    return res;
}

void set_flags(Machine* machine, bool n, bool z, bool p) {
    if (n) machine->PSR |= 0b0000000000000100; 
    else   machine->PSR &= 0b1111111111111011;

    if (z) machine->PSR |= 0b0000000000000010;
    else   machine->PSR &= 0b1111111111111101;

    if (p) machine->PSR |= 0b0000000000000001;
    else   machine->PSR &= 0b1111111111111110;
}

void set_flags_from_result(Machine* machine, Word result) {
    if (result == 0) set_flags(machine, 0, 1, 0);
    else if ((result & 0b1000000000000000) == 0) set_flags(machine, 0, 0, 1);
    else set_flags(machine, 1, 0, 0); 
}


void print_bits(unsigned int num) {
    for(int bit = 0; bit < (sizeof(unsigned int) * 8); bit++) {
        printf("%i ", num & 0x01);
        num = num >> 1;
    }
}

void print_word(uWord word) {
    char res[16];
    for(int bit = ((sizeof(uWord) * 8)-1); bit >= 0; bit--) {
        if ((word & 1) != 0) {
            res[bit] = '1';
        } else {
            res[bit] = '0';
        }
        word = word >> 1;
    }
    printf("%s", res);
}

void op_add_reg(uWord rest, Machine *machine) {
    int DR_id  = (rest & 0b0000111000000000) >> 9;
    int SR1_id = (rest & 0b0000000111000000) >> 6;
    int SR2_id = (rest & 0b0000000000000111) >> 0;

    Word result = machine->registers[SR1_id] + machine->registers[SR2_id];
    set_flags_from_result(machine, result);
    machine->registers[DR_id] = result;
}

void op_add_imm(uWord rest, Machine *machine) {
    unsigned int DR_id = (rest & 0b0000111000000000) >> 9;
    unsigned int SR_id = (rest & 0b0000000111000000) >> 6;
    int IMM            = (rest & 0b0000000000011111) >> 0;

    Word result = machine->registers[SR_id] + sext(IMM, 5);
    set_flags_from_result(machine, result);
    machine->registers[DR_id] = result;
}

void op_and_reg(uWord rest, Machine *machine) {
    unsigned int DR_id  = (rest & 0b0000111000000000) >> 9;
    unsigned int SR1_id = (rest & 0b0000000111000000) >> 6;
    unsigned int SR2_id = (rest & 0b0000000000000111) >> 0;

    Word result = machine->registers[SR1_id] & machine->registers[SR2_id];
    set_flags_from_result(machine, result);
    machine->registers[DR_id] = result;
}

void op_and_imm(uWord rest, Machine *machine) {
    unsigned int DR_id = (rest & 0b0000111000000000) >> 9;
    unsigned int SR_id = (rest & 0b0000000111000000) >> 6;
    int IMM            = (rest & 0b0000000000011111) >> 0;

    Word result = machine->registers[SR_id] & sext(IMM, 5);
    set_flags_from_result(machine, result);
    machine->registers[DR_id] = result;
}

void op_not(uWord rest, Machine *machine) {
    unsigned int DR_id = (rest & 0b0000111000000000) >> 9;
    unsigned int SR_id = (rest & 0b0000000111000000) >> 6;

    Word result = ~(machine->registers[SR_id]);
    set_flags_from_result(machine, result);
    machine->registers[DR_id] = result;
}

void op_br(uWord rest, Machine* machine) {
    bool n = (rest & 0b0000100000000000) != 0;
    bool z = (rest & 0b0000010000000000) != 0;
    bool p = (rest & 0b0000001000000000) != 0;
    int offset = rest & 0b0000000111111111;  
    bool cond = n && ((machine->PSR & 0b0000000000000100) != 0) 
             || z && ((machine->PSR & 0b0000000000000010) != 0)
             || p && ((machine->PSR & 0b0000000000000001) != 0);
    if (cond) machine->PC += (int16_t)sext(offset, 9);
}

void op_jmp(uWord rest, Machine* machine) {
    uWord BaseR_id = rest & 0b0000000111000000; 
    BaseR_id >>= 6;

    machine->PC = machine->registers[BaseR_id];
}

void op_jsr(uWord rest, Machine* machine) {
    machine->registers[7] = machine->PC; 
    int offset = rest & 0b0000011111111111;

    machine->PC += sext(offset, 11);
}

void op_jsrr(uWord rest, Machine* machine) {
    machine->registers[7] = machine->PC; 
    uWord BaseR_id = rest & 0b0000000111000000;
    BaseR_id >>= 6;
    
    machine->PC = machine->registers[BaseR_id];
}

void op_ld(uWord rest, Machine* machine, Memory memory) {
    uWord DR_id  = (rest & 0b0000111000000000) >> 9; 
    uWord offset = (rest & 0b0000000111111111); 

    uint16_t result = memory[machine->PC + sext(offset, 9)];
    machine->registers[DR_id] = result;

    set_flags_from_result(machine, result);
}

void op_ldi(uWord rest, Machine* machine, Memory memory) {
    uWord DR_id  = (rest & 0b0000111000000000) >> 9; 
    uWord offset = (rest & 0b0000000111111111); 

    uWord addr = memory[machine->PC + sext(offset, 9)];

    Word result = (Word)memory[addr];
    machine->registers[DR_id] = result;

    set_flags_from_result(machine, result);
}

void op_ldr(uWord rest, Machine* machine, Memory memory) {
    uWord DR_id    = (rest & 0b0000111000000000) >> 9; 
    uWord BaseR_id = (rest & 0b0000000111000000) >> 6;
    uWord offset   = (rest & 0b0000000000111111); 

    uWord abs_addr = machine->registers[BaseR_id] + (offset);
    uWord result = memory[abs_addr] | memory[abs_addr + 1] << 8;
    machine->registers[DR_id] = result;

    set_flags_from_result(machine, result);
}

void op_lea(uWord rest, Machine* machine) {
    uWord DR_id  = (rest & 0b0000111000000000) >> 9; 
    uWord offset = (rest & 0b0000000111111111);

    uWord result = machine->PC + sext(offset, 9);
    machine->registers[DR_id] = result;

    set_flags_from_result(machine, result);
}

void op_st(uWord rest, Machine* machine, Memory memory) {
    uWord SR_id  = (rest & 0b0000111000000000) >> 9; 
    uWord offset = (rest & 0b0000000111111111); 

    memory[machine->PC + sext(offset, 9)] = machine->registers[SR_id];
}

void op_sti(uWord rest, Machine* machine, Memory memory) {
    uWord SR_id  = (rest & 0b0000111000000000) >> 9; 
    uWord offset = (rest & 0b0000000111111111); 

    uWord addr = memory[machine->PC + sext(offset, 9)];

    memory[addr] = machine->registers[SR_id];
}

void op_str(uWord rest, Machine* machine, Memory memory) {
    uWord SR_id    = (rest & 0b0000111000000000) >> 9; 
    uWord BaseR_id = (rest & 0b0000000111000000) >> 6;
    uWord offset   = (rest & 0b0000000000111111); 

    memory[machine->registers[BaseR_id] + sext(offset, 6)] = machine->registers[SR_id];
}

void op_rti(uWord rest, Machine* machine, Memory memory) {
    if ((machine->PSR & PSR_BIT_SSM) != 0) {
        machine->registers[6] = MEM_OSSPC_END;
        machine->registers[6] -= 2;
        memory[machine->registers[6] + 1] = machine->PSR;
        memory[machine->registers[6] + 2] = machine->PC;
        machine->PSR |= PSR_BIT_SSM;
        machine->PC = memory[VEC_PRIV_MODE_VIOLATION];
    }
    machine->PSR = memory[machine->registers[6]++];
    machine->PC = memory[machine->registers[6]++];
}

void op_trap(uWord rest, Machine* machine, Memory memory) {
    uint8_t trap_8 = rest & 0b11111111;
    if (trap_8 == 0x25) {
        program_should_close = true;
        printf("halting cpu\n");
        return;
    }

    uWord addr = memory[trap_8];

    machine->registers[6] = MEM_OSSPC_END;

    memory[--machine->registers[6]] = machine->PSR;
    memory[--machine->registers[6]] = machine->PC;

    machine->PSR |= PSR_BIT_SSM;
    machine->registers[7] = machine->PC;
    machine->PC = addr;
}

bool execute_instruction(Machine* machine, Instruction inst, Memory memory) {
    Op_Id op   = (inst & 0b1111000000000000) >> 12;
    uWord rest = (inst & 0b0000111111111111);

    switch (op) {
        case Op_BR: {
            op_br(rest, machine);
        } break;

        case Op_ADD: {
            bool flag = (rest & 0b0000000000100000) != 0;
            if (flag) {
                op_add_imm(rest, machine);
            } else {
                op_add_reg(rest, machine);
            }
        } break;

        case Op_LD: {
            op_ld(rest, machine, memory);
        } break;

        case Op_ST: {
            op_st(rest, machine, memory);
        } break;

        case Op_JSR: {
            bool flag = (rest & 0b0000100000000000) != 0;
            if (flag) {
               op_jsr(rest, machine);
            } else {
                op_jsrr(rest, machine);
            }
        } break;

        case Op_AND: {
            bool flag = (rest & 0b0000000000100000) != 0;
            if (flag) {
                op_and_imm(rest, machine);
            } else {
                op_and_reg(rest, machine);
            }
        } break;

        case Op_LDR: {
            op_ldr(rest, machine, memory);
        } break; 

        case Op_STR: {
            op_str(rest, machine, memory);
        } break;

        case Op_RTI: {
        } break;

        case Op_NOT: {
            op_not(rest, machine);
        } break;

        case Op_LDI: {
            op_ldi(rest, machine, memory);
        } break;

        case Op_STI: {
            op_sti(rest, machine, memory);
        } break;

        case Op_JMP: {
            op_jmp(rest, machine);
        } break;

        case Op_RES: {
            printf("[ERROR] Illegal Opcode\n");
            return false;
        } break;

        case Op_LEA: {
            op_lea(rest, machine);
        } break;

        case Op_TRAP: {
            op_trap(rest, machine, memory);
        } break;
    }
    return true;
}

void execute_program(Machine* machine, Memory memory) {
    while (!program_should_close) {
        if (machine->PC + 1 >= MEMORY_SIZE) {
            printf("End of Memory Reached\n");
            return;
        }
        uWord inst = memory[machine->PC++];
        bool res = execute_instruction(machine, inst, memory);
        if (!res) printf("ERROR: Instruction no %u\n", machine->PC);
    }
}

void print_byte_data(const Byte_Data byte_data) {
    for (int i = 0; i < byte_data.count; i++) {
        printf("%d:%x\n", i, byte_data.bytes[i]);
    }
}

void print_machine_state(const Machine* machine) {
    for (int i = 0; i < 8; i++) {
        printf("R%d:%d\n", i, machine->registers[i]);
    }
    printf("PC:0x%x\n", machine->PC);
    printf("PSR:%d\n", machine->PSR);
    printf("n:%d ",  (machine->PSR & 0b0000000000000001) != 0);
    printf("z:%d ",  (machine->PSR & 0b0000000000000010) != 0);
    printf("p:%d\n", (machine->PSR & 0b0000000000000100) != 0);
}

bool map_byte_data(Memory memory, const Byte_Data* byte_data, size_t loc) {
    if (loc + byte_data->count > MEMORY_SIZE) {
        printf("[ERROR] mapped data is too large for memory\n");
        return false;
    }
    memcpy(memory, byte_data->bytes, byte_data->count);
    return true;
}

char* read_file(const char* file_name) {
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
    return content;
}

Byte_Data read_bin_from_file(char* file_name) {
    FILE* fh;
    fh = fopen(file_name, "rb");
    if (!fh) {
        printf("[ERROR] could not open specified file `%s`", file_name);
        exit(1);
    }

    fseek(fh, 0, SEEK_END);
    size_t length = ftell(fh);
    fseek(fh, 0, SEEK_SET);

    Byte_Data byte_data = init_byte_data_size(length);
    byte_data.count = length;
    int ok = fread(byte_data.bytes, 1, length, fh);
    if (!ok) {
        printf("[ERROR] error reading binary data for file `%s`", file_name);
        exit(1);
    }

    return byte_data;
}

bool write_bin_to_file(const Byte_Data* byte_data, char* file_name) {
    FILE* file;
    file = fopen(file_name, "wb");
    if (file == NULL) return false;
    for (int i = 0; i < byte_data->count; i++) {
        fwrite(&byte_data->bytes[i], sizeof(uWord), 1, file);
    }
    fclose(file);
    return true;
}

int main(int argc, char** argv) {
    Machine machine = {0};
    Memory memory = {0};
    char* os_file_name;
    char* program_file_name;
    bool loados = false;
    bool loadprogram = false;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-b") == 0) {
            if (!argv[i+1]) {
                printf("Usage: -b <executable_file_path>\n");
                return -1;
            }
            loadprogram = true;
            program_file_name = argv[i+1];
        } 
        if (strcmp(argv[i], "-os") == 0) {
            if (!argv[i+1]) {
                printf("Usage: -os <os_file_path>\n");
                return -1;
            }
            loados = true;
            os_file_name = argv[i+1];
        } 
    }
    if (!loadprogram && !loados){
        printf("Please provide a program file name to load\n");
        printf("atleast one file is required to run the emulator\n");
        printf("for raw files: ");
        printf("   Usage: -b <executable_file_path>\n");
        printf("for os files: ");
        printf("   Usage: -os <os_file_path>\n");
        return -1;
    }
    if (loados) {
        Byte_Data os = read_bin_from_file(os_file_name);
        map_byte_data(memory, &os, MEM_OSSPC_BEGIN);
        machine.PC = MEM_OSSPC_BEGIN;
    } else {
        machine.PC = MEM_USERSPC_BEGIN;
    } 
    if (loadprogram) {
        Byte_Data byte_data = read_bin_from_file(program_file_name);
    }
    execute_program(&machine, memory);
    print_machine_state(&machine);
}


