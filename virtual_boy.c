#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define lengthof(arr) sizeof(arr)/sizeof(arr[0])

typedef uint16_t uWord;
typedef int16_t   Word;

#define MEMORY_SIZE 65535

typedef uWord Memory[MEMORY_SIZE];

// Actual layout:
// Trap Vector Table      : 0x0000 - 0x00FF
// Interrupt Vector Table : 0x0100 - 0x01FF
// OS Space               : 0x0200 - 0x2FFF
// User Space             : 0x3000 - 0xFDFF
// I/O Register Space     : 0xFE00 - 0xFFFF

enum {
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

typedef uWord Instruction;

typedef struct {
    Word  registers[8];
    uWord PC;
    uWord PSR;
} Machine;

typedef struct {
    size_t       capacity; 
    size_t       count; 
    Instruction* bytes;
} Byte_Data;

Byte_Data* init_byte_data() {
    Byte_Data* byte_data = malloc(sizeof(Byte_Data));
    byte_data->count = 0;
    byte_data->capacity = 5;
    byte_data->bytes = malloc(sizeof(uWord)*byte_data->capacity);
    return byte_data;
}

void push_data(Byte_Data* byte_data, uWord data) {
    if (byte_data->count+=1 > byte_data->capacity) {
        byte_data = realloc(byte_data, byte_data->capacity + 5); 
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
    else if ((result & 0b1000000000000000) == 0) set_flags(machine, 1, 0, 0); 
    else set_flags(machine, 0, 0, 1); 
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

void add_reg(uWord rest, Machine *machine) {
    int DR_id  = (rest & 0b0000111000000000) >> 9;
    int SR1_id = (rest & 0b0000000111000000) >> 6;
    int SR2_id = (rest & 0b0000000000000111) >> 0;

    Word result = machine->registers[SR1_id] + machine->registers[SR2_id];
    set_flags_from_result(machine, result);
    machine->registers[DR_id] = result;
}

void add_imm(uWord rest, Machine *machine) {
    unsigned int DR_id = (rest & 0b0000111000000000) >> 9;
    unsigned int SR_id = (rest & 0b0000000111000000) >> 6;
    int IMM            = (rest & 0b0000000000011111) >> 0;

    Word result = machine->registers[SR_id] + sext(IMM, 5);
    set_flags_from_result(machine, result);
    machine->registers[DR_id] = result;
}

void and_reg(uWord rest, Machine *machine) {
    unsigned int DR_id  = (rest & 0b0000111000000000) >> 9;
    unsigned int SR1_id = (rest & 0b0000000111000000) >> 6;
    unsigned int SR2_id = (rest & 0b0000000000000111) >> 0;

    Word result = machine->registers[SR1_id] & machine->registers[SR2_id];
    set_flags_from_result(machine, result);
    machine->registers[DR_id] = result;
}

void and_imm(uWord rest, Machine *machine) {
    unsigned int DR_id = (rest & 0b0000111000000000) >> 9;
    unsigned int SR_id = (rest & 0b0000000111000000) >> 6;
    int IMM            = (rest & 0b0000000000011111) >> 0;

    Word result = machine->registers[SR_id] & sext(IMM, 5);
    set_flags_from_result(machine, result);
    machine->registers[DR_id] = result;
}

void not(uWord rest, Machine *machine) {
    unsigned int DR_id = (rest & 0b0000111000000000) >> 9;
    unsigned int SR_id = (rest & 0b0000000111000000) >> 6;

    Word result = ~(machine->registers[SR_id]);
    set_flags_from_result(machine, result);
    machine->registers[DR_id] = result;
}

void br(uWord rest, Machine* machine) {
    bool n = (rest & 0b0000100000000000) != 0;
    bool z = (rest & 0b0000010000000000) != 0;
    bool p = (rest & 0b0000001000000000) != 0;
    int offset = rest & 0b0000000111111111;  
    bool cond = n && ((machine->PSR & 0b0000000000000100) == 0) 
             || z && ((machine->PSR & 0b0000000000000010) == 0)
             || p && ((machine->PSR & 0b0000000000000001) == 0);
    if (cond) machine->PC+=sext(offset, 9);
}

void jmp(uWord rest, Machine* machine) {
    uWord BaseR_id = rest & 0b0000000111000000; 
    BaseR_id = BaseR_id >> 6;

    machine->PC = machine->registers[BaseR_id];
}

void jsr(uWord rest, Machine* machine) {
    machine->registers[7] = machine->PC+1; 
    int offset = rest & 0b0000011111111111;

    machine->PC+=sext(offset, 11);
}

void jsrr(uWord rest, Machine* machine) {
    machine->registers[7] = machine->PC+1; 
    uWord BaseR_id = rest & 0b0000000111000000;
    BaseR_id = BaseR_id << 6;
    
    machine->PC = machine->registers[BaseR_id];
}

void ld(uWord rest, Machine* machine, Memory memory) {
    uWord DR_id  = (rest & 0b0000111000000000) >> 9; 
    uWord offset = (rest & 0b0000000111111111); 

    Word result = (Word)memory[machine->PC + sext(offset, 9)]; 
    machine->registers[DR_id] = result;

    set_flags_from_result(machine, result);
}

void ldi(uWord rest, Machine* machine, Memory memory) {
    uWord DR_id  = (rest & 0b0000111000000000) >> 9; 
    uWord offset = (rest & 0b0000000111111111); 

    uWord addr = memory[machine->PC + sext(offset, 9)];

    Word result = (Word)memory[addr];
    machine->registers[DR_id] = result;

    set_flags_from_result(machine, result);
}

void ldr(uWord rest, Machine* machine, Memory memory) {
    uWord DR_id    = (rest & 0b0000111000000000) >> 9; 
    uWord BaseR_id = (rest & 0b0000000111000000) >> 6;
    uWord offset   = (rest & 0b0000000000111111); 

    Word result = (Word)memory[machine->registers[BaseR_id] + sext(offset, 6)];
    machine->registers[DR_id] = result;

    set_flags_from_result(machine, result);
}

void lea(uWord rest, Machine* machine) {
    uWord DR_id  = (rest & 0b0000111000000000) >> 9; 
    uWord offset = (rest & 0b0000000111111111) >> 9; 

    Word result = machine->PC + sext(offset, 9);
    machine->registers[DR_id] = result;

    set_flags_from_result(machine, result);
}

void st(uWord rest, Machine* machine, Memory memory) {
    uWord SR_id  = (rest & 0b0000111000000000) >> 9; 
    uWord offset = (rest & 0b0000000111111111); 

    memory[machine->PC + sext(offset, 9)] = (uWord)machine->registers[SR_id];
}

void sti(uWord rest, Machine* machine, Memory memory) {
    uWord SR_id  = (rest & 0b0000111000000000) >> 9; 
    uWord offset = (rest & 0b0000000111111111); 

    uWord addr = memory[machine->PC + sext(offset, 9)];

    memory[addr] = machine->registers[SR_id];
}

void str(uWord rest, Machine* machine, Memory memory) {
    uWord SR_id    = (rest & 0b0000111000000000) >> 9; 
    uWord BaseR_id = (rest & 0b0000000111000000) >> 6;
    uWord offset   = (rest & 0b0000000000111111); 

    memory[machine->registers[BaseR_id] + sext(offset, 6)] = machine->registers[SR_id];
}


bool execute_instruction(Machine* machine, Instruction inst, Memory memory) {
    Op_Id op   = (inst & 0b1111000000000000) >> 12;
    uWord rest = (inst & 0b0000111111111111);

    switch (op) {
        case Op_BR: {
            br(rest, machine);
        } break;

        case Op_ADD: {
            bool flag = (rest & 0b0000000000100000) != 0;
            if (flag) {
                add_imm(rest, machine);
            } else {
                add_reg(rest, machine);
            }
        } break;

        case Op_LD: {
            ld(rest, machine, memory);
        } break;

        case Op_ST: {
            st(rest, machine, memory);
        } break;

        case Op_JSR: {
            bool flag = (rest & 0b0000100000000000) != 0;
            if (flag) {
                jsr(rest, machine);
            } else {
                jsrr(rest, machine);
            }
        } break;

        case Op_AND: {
            bool flag = (rest & 0b0000000000100000) != 0;
            if (flag) {
                add_imm(rest, machine);
            } else {
                add_reg(rest, machine);
            }
        } break;

        case Op_LDR: {
            ldr(rest, machine, memory);
        } break; 

        case Op_STR: {
            str(rest, machine, memory);
        } break;

        case Op_RTI: break;

        case Op_NOT: {
            not(rest, machine);
        } break;

        case Op_LDI: {
            ldi(rest, machine, memory);
        } break;

        case Op_STI: {
            sti(rest, machine, memory);
        } break;

        case Op_JMP: {
            jmp(rest, machine);
        } break;

        case Op_RES: {
            printf("ERROR: Illegal Opcode\n");
            return false;
        } break;

        case Op_LEA: {
            lea(rest, machine);
        } break;

        case Op_TRAP: break;
    }
    return true;
}

void execute_program(Machine* machine, Memory memory) {
    for (int i = 0; i < 2; i++) {
        uWord curr_inst = memory[machine->PC];
        if (machine->PC >= MEMORY_SIZE) {
            printf("End of Memory Reached\n");
            return;
        }
        machine->PC++;
        bool res = execute_instruction(machine, curr_inst, memory);
        if (!res) printf("ERROR: Instruction no: %d\n", i+1);
    }
}

void print_byte_data(const Byte_Data* byte_data) {
    for (int i = 0; i < byte_data->count; i++) {
        printf("%d: ", i);
        print_word(byte_data->bytes[i]);
        printf("\n");
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

bool map_byte_data(Memory memory, const Byte_Data* byte_data, size_t loc, size_t offset) {
    if (loc+offset + byte_data->count> MEMORY_SIZE) {
        printf("ERROR: mapped data is too large for memory\n");
        return false;
    }
    for (int i = 0; i < byte_data->count; i++) {
        memory[i+loc+offset] = byte_data->bytes[i];
    }
    return true;
}

int main() {
    Memory memory = {0};
    Machine machine = {0};
    Byte_Data* byte_data = init_byte_data();

    push_data(byte_data, 0b0001011000100001);
    push_data(byte_data, 0b0001001000100011);

    map_byte_data(memory, byte_data, MEM_USERSPC_BEGIN, 100);
    machine.PC = MEM_USERSPC_BEGIN + 100;
    execute_program(&machine, memory);

    print_machine_state(&machine);
}


