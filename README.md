# A Virtual Machine For LC3 Architecture
- Written completely in c
- Uses .dat extension for binary files

# Quick Start

## The VM
The virtual boy program is: `emulator/bin/vboy`.
And, the examples programs are in `examples/`, with the .dat extension.

```
./emulator/bin/vboy -b ./examples/add_two_numbers.dat
```
* __NOTE__: do not forget the `-b` flag

## The Byte Writer
There is no assembly to write programs for this virtual machine yet,
instead we use a header file, located in `./byte_writer/byte_writer.h`.
It contains all the functions in order to create a binary executables for the VM.

Here is one examples code, more can be found in `examples/`:
```
#include "../byte_writer.h"

#define OUTPUT_FILE "add_two_numbers.dat"

int main() {
    Byte_Data* byte_data = init_byte_data(); 

    push_data(byte_data, 0b0101000000100000); // REG0 & 0 -> REG1 (reset REG0 to 0)
    push_data(byte_data, 0b0001000000100101); // REG0(0) + 5 -> REG0 (set REG0 to 5)
    // this is how you mov a value, you first reset it to 0 and then add the value what you want

    push_data(byte_data, 0b0101001001100000); // REG1 & 0 -> REG1 (reset reg1 to 0)
    push_data(byte_data, 0b0001001001100010); // REG1(0) + 5 -> REG1 (set REG1 to 5)
    // doing the same for REG1, now REG1 = 2

    push_data(byte_data, 0b0001010000000001); // REG0(5) + REG1(2) -> REG2(7)
    // simple addition

    push_data(byte_data, 0b1111000000000000); 
    // HALT instruction to stop the program 

    write_bin_to_file(byte_data, OUTPUT_FILE); // visualizing the machine state
    free(byte_data);
}
```
* __NOTE__: do not forget the `-b` flag
