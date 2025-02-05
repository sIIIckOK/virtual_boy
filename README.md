
# A Virtual Machine For LC3 Architecture
- Written completely in c
- Uses .dat extension for binary files

## References
- [About Instruction Set](https://www.cs.colostate.edu/~cs270/.Fall18/resources/PattPatelAppA.pdf)

## Important Notes
- Interrupts and Traps are not supported yet, except for `1111000000000000` which is the HALT instruction.

# Quick Start

## The VM
The virtual boy program is: `emulator/bin/vboy`.
And, the examples programs are in `examples/`, with the .bin extension.

```bash
./emulator/bin/vboy -b ./examples/add_two_numbers.dat
```
* __NOTE__: do not forget the `-b` flag

## The Assembler
There is an assembler that is a part of this project that is currently in development
here is the basic syntax:
```asm
add %r0 $r0 #1
```

### Registers
registers are prefixed with `%`\
there is r0-r7 general purpose registers

### Int Literals (only decimal supported for now)
int literals (only decimals supported for now) are prefixed with `#`\
negetive literals would be `#-1` for example

### Labels
labels are prefixed with `$`\
label definitions would look like this `$label:` where the `:` is required.\
__Note__ that the current iteration of the assembler cannot warn you about label definitions without the trailing `:` so be careful
when you want to refer to a label you use `$label` without the `:` in an instruction if would look like
```asm
$label:
br nzp $label
```

### Compared to Conventional Assembly Syntax
this assembler is designed the way it is for the simplicity of implementation, thats why the syntax feels weird.\
For example compared to conventional assemblers that are new line sensitive, where each instruction needs to be in a new line,\
this assembler treats new lines and spaces the same, so in theory you can write multiple instructions in the same line.\

## The Byte Writer (__About to be deprecated__)
There is no assembly to write programs for this virtual machine yet,\
instead we use a header file, located in `byte_writer/byte_writer.h`.\
It contains all the functions in order to create a binary executables for the VM.  

Here is one examples code, more can be found in `examples/`:
```c
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
