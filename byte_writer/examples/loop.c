#include "../byte_writer.h"

#define OUTPUT_FILE "loop.dat"

int main() {
    Byte_Data* byte_data = init_byte_data(); 

    push_data(byte_data, 0b0001101101100101); // set REG5 = 5
    push_data(byte_data, 0b0001000000100010); // add REG0 += 2 every iteration // 11 00 01
    push_data(byte_data, 0b0001101101111111); // decrement REG5(counter) by 1
    push_data(byte_data, 0b0000001111111101); // branch if the last instruction yielded a positive result
    // pc -> +0
    push_data(byte_data, 0b1111000000000000); // branch if the last instruction yielded a positive result

    write_bin_to_file(byte_data, OUTPUT_FILE); // visualizing the machine state
    free(byte_data);
}


