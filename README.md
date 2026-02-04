This repository contains two main projects: an ARM64 assembler and a DFA (Deterministic Finite Automaton) recognizer.

---

## DFA Recognizer

A deterministic finite automaton (DFA) recognizer written in C++ that reads a DFA specification and evaluates whether input strings are accepted by the automaton.

### Overview

The DFA recognizer (`dfa` / `dfa.cpp`) reads a DFA definition including its alphabet, states, transitions, and input strings, then determines whether each input string is accepted or rejected by the DFA.

### Input Format

The DFA specification file consists of four sections:

#### 1. `.ALPHABET`
Defines the set of symbols the DFA recognizes. Symbols can be specified as:
- Individual characters: `a`, `b`, `!`
- Character ranges: `a-z`, `A-Z`, `0-9`

Example:
```
.ALPHABET
a-z A-Z 0-9
```

#### 2. `.STATES`
Lists all states in the DFA. The first state is the initial state. Accepting (final) states are marked with a `!` suffix.

Example:
```
.STATES
start
reading!
error
```

In this example, `start` is the initial state and `reading` is an accepting state.

#### 3. `.TRANSITIONS`
Defines state transitions. Each line specifies:
```
<from_state> <symbol(s)> <to_state>
```

Symbols can be individual characters or ranges. Multiple symbols can transition to the same state.

Example:
```
.TRANSITIONS
start a-z reading
reading a-z reading
reading 0-9 error
```

#### 4. `.INPUT`
Lists input strings to be tested. Each string is evaluated and the program outputs whether it's accepted or rejected. Use `.EMPTY` to represent an empty string.

Example:
```
.INPUT
hello
world
.EMPTY
abc123
```

### Building

Compile the DFA recognizer using g++:

```bash
g++ -std=c++17 -o dfa dfa.cpp
```

### Usage

```bash
./dfa < input.dfa
```

The program reads from standard input and outputs results to standard output.

### Output Format

For each input string, the program outputs:
```
<string> true
```
or
```
<string> false
```

Where `true` indicates the string is accepted by the DFA, and `false` indicates rejection.

### Example

Given `input.dfa`:
```
.ALPHABET
a-z A-Z !
.STATES
start!
.TRANSITIONS
start a-z A-Z ! start
.INPUT
Words are wonderful!
Accept these words into your heart!
.EMPTY
Computer Science
```

Running `./dfa < input.dfa` outputs:
```
Words are wonderful! true
Accept these words into your heart! true
.EMPTY true
Computer Science false
```

The first three strings are accepted because they contain only letters, spaces, and exclamation marks. The last string is rejected because it contains a space (which is not in the alphabet).

### Implementation Details

- The DFA is stored using STL data structures:
  - `std::set<char>` for the alphabet
  - `std::set<std::string>` for accepting states
  - `std::map<std::pair<std::string, char>, std::string>` for transitions
- String evaluation is performed by simulating the DFA state transitions
- If no valid transition exists for a character, the string is rejected

---

## ARM Assembler

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