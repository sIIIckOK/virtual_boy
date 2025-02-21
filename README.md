# NOTES 
this is an incomplete project that made for education purposes, so its not supposed to be used in serious settings.  

# A Virtual Machine For LC3 Architecture
- Written completely in c
- Uses .dat extension for binary files

## References
- [About Instruction Set](https://www.cs.colostate.edu/~cs270/.Fall18/resources/PattPatelAppA.pdf)

# Quick Start

## The VM
The virtual boy program is: `emulator/bin/vboy`.
And, the examples programs are in `examples/`, with the .bin extension.

```bash
./emulator/bin/vboy -b ./testout/print.bin
```

You can also specify a custom os like this:  
```bash
./emulator/bin/vboy -os ./os.s -b ./testout/print.bin
```

## The Assembler
There is an assembler that is a part of this project  
look at assembler/assembler.c for the source  
for the assembler syntax see os.s  

here is the basic syntax:
```asm
add %r0 %r0 #1
```

### Registers
registers are prefixed with `%`\
there is r0-r7 general purpose registers

### Int Literals
int literals (only decimals supported for now) are prefixed with `#`\
negetive literals would be `#-1` for example  
for hex literals use `#x`, so `#xff` for example  

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
this assembler treats new lines and spaces the same, so in theory you can write multiple instructions in the same line.

__NOTE__: the Byte Writer is deprecated and is no longer used to make binaries, use the assembler instead  

