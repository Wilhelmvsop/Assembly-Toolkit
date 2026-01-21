# ARM Assembler

This is a simple ARM64 assembler written in C++ that compiles ARM assembly instructions into machine code. It reads assembly code from a file or standard input and outputs the corresponding binary machine code to standard output.

## Supported Instructions

The assembler supports the following ARM64 instructions:

- **Arithmetic Operations**:
  - `add rd, rn, rm` - Add registers
  - `sub rd, rn, rm` - Subtract registers
  - `mul rd, rn, rm` - Multiply registers
  - `smulh rd, rn, rm` - Signed multiply high
  - `umulh rd, rn, rm` - Unsigned multiply high
  - `sdiv rd, rn, rm` - Signed divide
  - `udiv rd, rn, rm` - Unsigned divide

- **Comparison**:
  - `cmp rn, rm` - Compare registers (equivalent to subs xzr, rn, rm)

- **Branch Operations**:
  - `br rn` - Branch to register
  - `blr rn` - Branch with link to register
  - `b imm` - Branch to immediate address (must be multiple of 4 bytes)

- **Load/Store Operations**:
  - `ldur rd, [rn, imm]` - Load register from memory with unscaled offset
  - `stur rd, [rn, imm]` - Store register to memory with unscaled offset
  - `ldr rd, imm` - Load register from PC-relative address (must be multiple of 4 bytes)

## Syntax

- Registers: `x0` to `x30`, `xzr` (zero register, only allowed where specified)
- Immediates: Decimal numbers (e.g., `42`), negative numbers (e.g., `-8`), or hexadecimal (e.g., `0xFF`)
- Comments: Lines starting with `//` or `;` are ignored
- Empty lines are ignored

## Building

Compile the assembler using g++:

```bash
g++ -o asm asm.cc
```

## Usage

```bash
./asm [input_file]
```

- If `input_file` is provided, reads assembly from that file
- If no argument or `-` is provided, reads from standard input
- Outputs binary machine code to standard output
- Errors are printed to standard error

## Examples

### Example 1: Simple addition

Create a file `add.arm` with:

```
add x0, x1, x2
```

Run:

```bash
./asm add.arm > add.bin
```

### Example 2: Load and store

```
ldur x0, [x1, 8]
stur x0, [x2, -16]
```

### Example 3: Branch

```
b 1024
```

## Error Handling

The assembler performs validation on:

- Instruction names
- Register ranges (x0-x30, xzr where allowed)
- Immediate value ranges (e.g., ldur/stur: -256 to 255, ldr/b: specific multiples)
- Parameter counts and types

Invalid assembly will print an error message to stderr and exit with code 1.

## Notes

- Output is in big-endian byte order
- All immediates for branch/load instructions must be multiples of 4 bytes where specified
- The assembler is designed for educational purposes in CS241