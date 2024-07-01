
# A Virtual Machine For LC3 Architecture
- Written completely in c
- Uses .dat extension for binary files

## References
- [About Instruction Set](https://www.cs.colostate.edu/~cs270/.Fall18/resources/PattPatelAppA.pdf)

## Important Notes
- Interrupts and Traps are supported except for `1111000000000000` which is the HALT instruction

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
instead we use a header file, located in `byte_writer/byte_writer.h`.
It contains all the functions in order to create a binary executables for the VM.

Here is one examples code, more can be found in `examples/`:
```
#include "../byte_writer.h"

#define OUTPUT_FILE "add_two_numbers.dat"

int main() {
    Byte_Data* byte_data = init_byte_data(); 

                         // instruction
    push_data(byte_data, 0b0101000000100000);
    push_data(byte_data, 0b0001000000100101);

    push_data(byte_data, 0b0101001001100000);
    push_data(byte_data, 0b0001001001100010);

    push_data(byte_data, 0b0001010000000001);

    // HALT:
    push_data(byte_data, 0b1111000000000000); 

    write_bin_to_file(byte_data, OUTPUT_FILE);
    free(byte_data);
}
```
