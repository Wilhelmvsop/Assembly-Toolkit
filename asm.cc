#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <map>
#include <cstdint>
#include <stdexcept>

void formatError(const std::string &message);

/** For a given instruction, returns the machine code for that instruction.
 *
 * @param[out] word The machine code for the instruction
 * @param instruction The name of the instruction
 * @param one The value of the first parameter
 * @param two The value of the second parameter
 * @param three The value of the third parameter
 */

static uint32_t imm_mod_two(int imm, uint32_t mod)
{
    long long x = static_cast<long long>(imm);
    long long m = static_cast<long long>(mod);
    long long r = x % m;
    if (r < 0) r += m;
    return static_cast<uint32_t>(r);
}

static uint32_t bswap32_arith(uint32_t enc)
{
    uint32_t b0 = enc % 256u;
    uint32_t b1 = (enc / 256u) % 256u;
    uint32_t b2 = (enc / 65536u) % 256u;
    uint32_t b3 = (enc / 16777216u) % 256u;

    return b0 * 16777216u + b1 * 65536u + b2 * 256u + b3;
}

static uint32_t enc_r3_arith(uint32_t base, int rd, int rn, int rm)
{
    return base
         + static_cast<uint32_t>(rm) * 65536u   // 2^16
         + static_cast<uint32_t>(rn) * 32u      // 2^5
         + static_cast<uint32_t>(rd);
}

bool compileLine(uint32_t &          word,
                 const std::string & instruction,
                 int                 one,
                 int                 two,
                 int                 three)
{
    uint32_t enc = 0;

    if (instruction == "add")
    {
        enc = enc_r3_arith(0x8B206000u, one, two, three);
    }
    else if (instruction == "br")
    {
        enc = 0xD61F0000u + static_cast<uint32_t>(one) * 32u;
    }
    else if (instruction == "blr")
    {
        enc = 0xD63F0000u + static_cast<uint32_t>(one) * 32u;
    }
    else if (instruction == "cmp")
    {
        // cmp xN, xM  == subs xzr, xN, xM 
        enc = enc_r3_arith(0xEB206000u, 31, one, two);
    }
    else if (instruction == "sub")
    {
        enc = enc_r3_arith(0xCB206000u, one, two, three);
    }
    else if (instruction == "mul")
    {
        enc = enc_r3_arith(0x9B007C00u, one, two, three);
    }
    else if (instruction == "smulh")
    {
        enc = enc_r3_arith(0x9B407C00u, one, two, three);
    }
    else if (instruction == "umulh")
    {
        enc = enc_r3_arith(0x9BC07C00u, one, two, three);
    }
    else if (instruction == "sdiv")
    {
        enc = enc_r3_arith(0x9AC00C00u, one, two, three);
    }
    else if (instruction == "udiv")
    {
        enc = enc_r3_arith(0x9AC00800u, one, two, three);
    }
    
    
    else if (instruction == "ldur")
    {
        int imm = three;
        if (imm < -256 || imm > 255)
        {
            formatError("ldur immediate out of range : " + std::to_string(imm));
            return false;
        }
        uint32_t imm9 = imm_mod_two(imm, 512u); // 2^9
        enc = 0xF8400000u
            + imm9 * 4096u
            + static_cast<uint32_t>(two) * 32u
            + static_cast<uint32_t>(one);
    }
    else if (instruction == "stur")
    {
        int imm = three;
        if (imm < -256 || imm > 255)
        {
            formatError("stur immediate out of range : " + std::to_string(imm));
            return false;
        }
        uint32_t imm9 = imm_mod_two(imm, 512u); // 2^9
        enc = 0xF8000000u
            + imm9 * 4096u
            + static_cast<uint32_t>(two) * 32u
            + static_cast<uint32_t>(one);
    }
    else if (instruction == "ldr")
    {
        int imm = two;
        if ((imm % 4) != 0)
        {
            formatError("ldr immediate must be a multiple of 4 bytes: " + std::to_string(imm));
            return false;
        }

        int imm19_signed = imm / 4;
        if (imm19_signed < -262144 || imm19_signed > 262143)
        {
            formatError("ldr immediate out of range : " + std::to_string(imm));
            return false;
        }

        uint32_t imm19 = imm_mod_two(imm19_signed, 524288u); // 2^19
        enc = 0x58000000u
            + imm19 * 32u
            + static_cast<uint32_t>(one);
    }
    else if (instruction == "b")
    {
        int imm = one;
        if ((imm % 4) != 0)
        {
            formatError("b immediate must be a multiple of 4 bytes: " + std::to_string(imm));
            return false;
        }

        int imm26_signed = imm / 4;
        if (imm26_signed < -33554432 || imm26_signed > 33554431)
        {
            formatError("b immediate out of range : " + std::to_string(imm));
            return false;
        }

        uint32_t imm26 = imm_mod_two(imm26_signed, 67108864u); // 2^26
        enc = 0x14000000u + imm26;
    }

    word = bswap32_arith(enc);
    return true;
}


/** Prints an error to stderr with an "ERROR: " prefix, and newline suffix.
 *
 * @param message The error to print
 */
void formatError(const std::string &message)
{
    std::cerr << "ERROR: " << message << std::endl;
}

/** Matches a line of ARM assembly, potentially with comments */
std::regex ARM_LINE_PATTERN(
    "^\\s*([a-z]+)\\s+(x\\d+|0x[0-9a-fA-F]*|-?\\d+|xzr)\\s*(?:(?:(?:,\\s*(x\\d+|0x[0-9a-fA-F]*|-?\\d+|xzr))?(?:,\\s*(x\\d+|0x[0-9a-fA-F]*|-?\\d+|xzr))?)|(?:,\\s*\\[\\s*(x\\d+|xzr)\\s*,\\s*(0x[0-9a-fA-F]*|-?\\d+)\\s*\\]))\\s*(?:\\/\\/.*)?$");

/** Recognizes an empty line (or an empty line with a comment) */
std::regex EMPTY_LINE("^\\s*(//.*)?$");

/** Maps the instruction name to the parameter type.  The value must be a 3 character string, 'r'
 *  represents a register, 'i' represents an immediate, 'z' represents a register where 0 is allowed, and ' ' represents no value */
const std::map<std::string, std::string> INSTRUCTION_PARAMETER_PATTERN = {
    {"add", "rrz"},
    {"sub", "rrz"},
    {"mul", "rrz"},
    {"smulh", "rrz"},
    {"umulh", "rrz"},
    {"sdiv", "rrz"},
    {"udiv", "rrz"},
    {"cmp", "rz "},
    {"br", "r  "},
    {"blr", "r  "},
    {"ldur", "rri"},
    {"stur", "rri"},
    {"ldr", "ri "},
    {"b", "i  "}};

/** Convert a string representation of an immediate value to a signed 32-bit integer. Accounts for negatives.
 * If the string starts with "0x", it is interpreted as an unsigned hexadecimal value.
 *
 * The function name is read as "string to uint32".
 *
 * @param s The string to parse
 * @return The uint32_t representation of the string
 */
int readImm(const std::string &s)
{
    if (s.starts_with("0x"))
    {
        return std::stoi(s.substr(2), nullptr, 16);
    }
    return std::stoi(s);
}

/** Convert a string representation of a register name to the register number. If "xzr" (the zero register)
 * is read, it returns 31. Otherwise, it returns the register number directly.
 *
 * The function name is read as "string to uint32".
 *
 * @param s The string to parse
 * @return The uint32_t representation of the string
 */
uint32_t readReg(bool zeroable, const std::string &s)
{
    if (s == "xzr")
    {
        if (zeroable)
        {
            return 31;
        }
        else
        {
            throw std::runtime_error("Register 'xzr' is not allowed in a non-'xm' position");
        }
    }

    if (!s.starts_with("x"))
    {
        throw std::runtime_error("Invalid register value '" + s + "'");
    }
    int ret = std::stoi(s.substr(1), nullptr, 10);
    if (ret > 30)
    {
        throw std::runtime_error("Register value '" + s + "' is too large");
    }
    if (ret < 0)
    {
        throw std::runtime_error("Register value '" + s + "' is negative");
    }
    return ret;
}

/** Compiles one line of assembly and send the binary to standard out.  If the assembly is invalid,
 *  print an error to stderr and return false.  Assumes that the assembly does not have a trailing
 *  comment.
 *
 * @param line The line to parse
 * @return True if the line is valid assembly and was output to stdout, false otherwise
 */
bool parseLine(const std::string &line)
{
    std::smatch matches;
    if (!std::regex_search(line, matches, ARM_LINE_PATTERN))
    {
        formatError((std::stringstream() << "Unable to parse line: \"" << line << "\"").str());
        return false;
    }

    std::string instruction = matches[1];

    uint32_t parameters[3] = {0, 0, 0};

    auto pattern = INSTRUCTION_PARAMETER_PATTERN.find(instruction);
    if (pattern == INSTRUCTION_PARAMETER_PATTERN.end())
    {
        formatError((std::stringstream() << "'" << instruction << "' is not a known instruction").str());
        return false;
    }

    std::string argmatches[3] = {
        matches[2],
        matches[3].matched ? matches[3] : matches[5],
        matches[4].matched ? matches[4] : matches[6],
    };
    uint32_t index = 0;
    try
    {
        for (char c : pattern->second)
        {
            if (c == 'r')
            {
                if (argmatches[index] == "")
                {
                    throw std::runtime_error("Instruction '" + instruction + "' is missing a register value");
                }
                parameters[index] = readReg(false, argmatches[index]);
            }
            else if (c == 'z')
            {
                if (argmatches[index] == "")
                {
                    throw std::runtime_error("Instruction '" + instruction + "' is missing a zeroable register value");
                }
                parameters[index] = readReg(true, argmatches[index]);
            }
            else if (c == 'i')
            {
                if (argmatches[index] == "")
                {
                    throw std::runtime_error("Instruction '" + instruction + "' is missing an immediate value");
                }
                parameters[index] = readImm(argmatches[index]);
            }
            else
            { // Extra args
                if (argmatches[index] != "")
                {
                    throw std::runtime_error("Instruction '" + instruction + "' has extraneous arguments");
                }
            }
            index++;
        }
    }
    catch (std::runtime_error &s)
    {
        formatError(s.what());
        return false;
    }
    catch (std::invalid_argument &arg)
    {
        formatError(arg.what());
        return false;
    }

    uint32_t binary = 0;
    bool compiled = compileLine(binary,
                                instruction,
                                parameters[0],
                                parameters[1],
                                parameters[2]);
    if (compiled)
    {
        // Output of the binary in BIG-ENDIAN order
        std::cout << (char)((binary >> 24) & 0xFF)
                  << (char)((binary >> 16) & 0xFF)
                  << (char)((binary >> 8) & 0xFF)
                  << (char)((binary >> 0) & 0xFF);
        return true;
    }
    else
    {
        // compileLine (should have) printed an error.  We don't have to print one here.
        return false;
    }
}

/** Entrypoint for the assembler.  The first parameter (optional) is a mips assembly file to
 *  read.  If no parameter is specified, read assembly from stdin.  Prints machine code to stdout.
 *  If invalid assembly is found, prints an error to stderr, stops reading assembly, and return a
 *  non-0 value.
 *
 * If the file is not found, print an error and returns a non-0 value.
 *
 * @return 0 on success, non-0 on error
 */
int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        std::cerr << "Usage:" << std::endl
                  << "\tasm [$FILE]" << std::endl
                  << std::endl
                  << "If $FILE is unspecified or if $FILE is `-`, read the assembly from standard "
                  << "in. Otherwise, read the assembly from $FILE." << std::endl;
        return 1;
    }

    std::ifstream fp;
    std::istream &in =
        (argc > 1 && std::string(argv[1]) != "-")
        ? [&]() -> std::istream &
    {
        fp.open(argv[1]);
        return fp;
    }()
        : std::cin;

    if (!fp && argc > 1)
    {
        formatError((std::stringstream() << "file '" << argv[1] << "' not found!").str());
        return 1;
    }

    while (!in.eof())
    {
        std::string line;
        std::getline(in, line);

        // Filter out any comments
        uint32_t startComment = line.find(";");
        if (startComment != std::string::npos)
        {
            line = line.substr(0, line.find(";"));
        }

        std::smatch matches;
        if (std::regex_search(line, matches, EMPTY_LINE))
        {
            continue;
        }

        if (!parseLine(line))
        {
            return 1;
        }
    }

    return 0;
}
