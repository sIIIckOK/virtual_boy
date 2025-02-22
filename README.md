# NOTES 
this is an incomplete project that made for education purposes, so its not supposed to be used in serious settings.  

# A Emulator For LC3 Architecture
- Written completely in c

## References
- [About Instruction Set](https://www.cs.colostate.edu/~cs270/.Fall18/resources/PattPatelAppA.pdf)

# Quick Start

## The Emulator
The virtual boy program is: `emulator/bin/vboy`.
And, the examples programs are in `examples/`, with the .bin extension.

Start by compiling to emulator. I am using gcc here, use whatever c compiler u like  
```bash
gcc ./emulator/virtual_boy.c -o ./vboy
```

Then you can provide a assembled file like this, with the `-b` flag  
```bash
./vboy -b ./testout/print.bin
```
__NOTE__: this will use the default `./os.bin` file as an operating system from the current directory of the execuatable  

If you want to use a custom os, do this:  
```bash
./vboy -os ./os.s -b ./testout/print.bin
```

## The Assembler

Start by compiling to assembler
```bash
gcc ./assembler/assembler.c -o ./assembler
```

Then use the assembler to make a `.bin` for your assembly program  
__NOTE__: You can consult the `os.s` and the `./examples/` directory for examples on the assembly  
```bash
./assembler ./examples/print.s -o ./out/print.bin
```

here is the basic syntax:  
```asm
add %r0 %r0 #1
```

### Registers
registers are prefixed with `%`  
there is r0-r7 general purpose registers

### Int Literals
int literals (only decimals supported for now) are prefixed with `#`  
negetive literals would be `#-1` for example  
for hex literals use `#x` and `#-x` for negetive hexadecimals, so `#xff` and `#-xff` for example  

### Labels
labels are prefixed with `$`  
label definitions would look like this `$label:` where the `:` is required.  
__Note__ that the current iteration of the assembler cannot warn you about label definitions without the trailing `:` so be careful  
when you want to refer to a label you use `$label` without the `:` in an instruction if would look like  
```asm
$label:
br nzp $label
```

### Compared to Conventional Assembly Syntax
this assembler is designed the way it is for the simplicity of implementation, thats why the syntax feels weird.  
For example compared to conventional assemblers that are new line sensitive, where each instruction needs to be in a new line,  
this assembler treats new lines and spaces the same, so in theory you can write multiple instructions in the same line.  

__NOTE__: the Byte Writer is deprecated and is no longer used to make binaries, use the assembler instead  

