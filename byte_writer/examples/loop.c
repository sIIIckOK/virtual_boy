#include "../byte_writer.h"

#define OUTPUT_FILE "loop.dat"

int main() {
    Byte_Data* byte_data = init_byte_data(); 

    push_data(byte_data, 0b0001101101100101); 
    push_data(byte_data, 0b0001000000100010);
    push_data(byte_data, 0b0001101101111111);
    push_data(byte_data, 0b0000001111111101);
    push_data(byte_data, 0b1111000000000000); // HALT

    write_bin_to_file(byte_data, OUTPUT_FILE); // ouput the binary
    free(byte_data);
}


