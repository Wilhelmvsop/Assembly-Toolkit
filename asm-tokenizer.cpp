#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <cstdint>
#include <vector>

/** Prints an error to stderr with an "ERROR: " prefix, and newline suffix. Terminates the program with an error.
 *
 * @param message The error to print
 */
void formatError(const std::string &message)
{
    throw std::runtime_error(message);
}

enum TokenType
{
    // Not a real token type we output: a unique value for initializing a TokenType when the
    // actual value is unknown
    NONE,

    DOTID,
    LABEL,
    ID,
    HEXINT,
    REG,
    ZREG,
    INT,
    COMMA,
    LBRACK,
    RBRACK,
    NEWLINE
};

struct Token
{
    TokenType type;
    std::string lexeme;
};

#define TOKEN_TYPE_READER(t) \
    if (s == #t)             \
    return t
TokenType stringToTokenType(const std::string &s)
{
    TOKEN_TYPE_READER(DOTID);
    TOKEN_TYPE_READER(LABEL);
    TOKEN_TYPE_READER(ID);
    TOKEN_TYPE_READER(HEXINT);
    TOKEN_TYPE_READER(REG);
    TOKEN_TYPE_READER(ZREG);
    TOKEN_TYPE_READER(INT);
    TOKEN_TYPE_READER(COMMA);
    TOKEN_TYPE_READER(LBRACK);
    TOKEN_TYPE_READER(RBRACK);
    TOKEN_TYPE_READER(NEWLINE);
    return NONE;
}
#undef TOKEN_TYPE_READER

#define TOKEN_TYPE_PRINTER(t) \
    case t:                   \
        return #t
std::string tokenTypeToString(const TokenType &t)
{
    switch (t)
    {
        TOKEN_TYPE_PRINTER(DOTID);
        TOKEN_TYPE_PRINTER(LABEL);
        TOKEN_TYPE_PRINTER(ID);
        TOKEN_TYPE_PRINTER(HEXINT);
        TOKEN_TYPE_PRINTER(REG);
        TOKEN_TYPE_PRINTER(ZREG);
        TOKEN_TYPE_PRINTER(INT);
        TOKEN_TYPE_PRINTER(COMMA);
        TOKEN_TYPE_PRINTER(LBRACK);
        TOKEN_TYPE_PRINTER(RBRACK);
        TOKEN_TYPE_PRINTER(NEWLINE);
    default:
        formatError("Unrecognized token type");
        return "";
    }
    return "NONE";
}
#undef TOKEN_TYPE_PRINTER

std::ostream &operator<<(std::ostream &out, const Token token)
{
    out << tokenTypeToString(token.type) << " " << token.lexeme;
    return out;
}

std::istream &operator>>(std::istream &in, Token &token)
{
    std::string tokenType;
    in >> tokenType;
    token.type = stringToTokenType(tokenType);
    if (token.type != NEWLINE)
    {
        in >> token.lexeme;
    }
    else
    {
        token.lexeme = "";
    }
    return in;
}

static uint32_t imm_mod_two(int imm, uint32_t mod)
{
    long long x = static_cast<long long>(imm);
    long long m = static_cast<long long>(mod);
    long long r = x % m;
    if (r < 0)
        r += m;
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
    return base + static_cast<uint32_t>(rm) * 65536u // 2^16
           + static_cast<uint32_t>(rn) * 32u         // 2^5
           + static_cast<uint32_t>(rd);
}

static bool decodeBCond(const std::string &instruction, uint32_t &cond)
{
    if (instruction.size() < 4 || instruction[0] != 'b' || instruction[1] != '.')
    {
        return false;
    }

    const std::string suffix = instruction.substr(2);
    if (suffix == "eq")
        cond = 0u;
    else if (suffix == "ne")
        cond = 1u;
    else if (suffix == "hs")
        cond = 2u;
    else if (suffix == "lo")
        cond = 3u;
    else if (suffix == "hi")
        cond = 8u;
    else if (suffix == "ls")
        cond = 9u;
    else if (suffix == "ge")
        cond = 10u;
    else if (suffix == "lt")
        cond = 11u;
    else if (suffix == "gt")
        cond = 12u;
    else if (suffix == "le")
        cond = 13u;
    else
        return false;

    return true;
}

static bool instructionAllowsLabelOperand(const std::string &instruction)
{
    if (instruction == "b")
    {
        return true;
    }
    uint32_t cond = 0;
    return decodeBCond(instruction, cond);
}

bool compileLine(uint32_t &word, const std::string &instruction, int one, int two, int three)
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
        uint32_t imm9 = imm_mod_two(imm, 512u);
        enc = 0xF8400000u + imm9 * 4096u + static_cast<uint32_t>(two) * 32u + static_cast<uint32_t>(one);
    }
    else if (instruction == "stur")
    {
        int imm = three;
        if (imm < -256 || imm > 255)
        {
            formatError("stur immediate out of range : " + std::to_string(imm));
            return false;
        }
        uint32_t imm9 = imm_mod_two(imm, 512u);
        enc = 0xF8000000u + imm9 * 4096u + static_cast<uint32_t>(two) * 32u + static_cast<uint32_t>(one);
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
        uint32_t imm19 = imm_mod_two(imm19_signed, 524288u);
        enc = 0x58000000u + imm19 * 32u + static_cast<uint32_t>(one);
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
        uint32_t imm26 = imm_mod_two(imm26_signed, 67108864u);
        enc = 0x14000000u + imm26;
    }
    else
    {
        uint32_t cond = 0;
        if (decodeBCond(instruction, cond))
        {
            int imm = one;
            if ((imm % 4) != 0)
            {
                formatError(instruction + " immediate must be a multiple of 4 bytes: " + std::to_string(imm));
                return false;
            }
            int imm19_signed = imm / 4;
            if (imm19_signed < -262144 || imm19_signed > 262143)
            {
                formatError(instruction + " immediate out of range : " + std::to_string(imm));
                return false;
            }
            uint32_t imm19 = imm_mod_two(imm19_signed, 524288u);
            enc = 0x54000000u + imm19 * 32u + cond;
        }
        else
        {
            formatError("Unknown instruction: " + instruction);
            return false;
        }
    }

    word = bswap32_arith(enc);
    return true;
}

int64_t parseInteger(const std::string &lexeme)
{
    if (lexeme.substr(0, 2) == "0x" || lexeme.substr(0, 2) == "0X")
    {
        return std::stoll(lexeme.substr(2), nullptr, 16);
    }
    return std::stoll(lexeme);
}

int parseRegister(const std::string &lexeme)
{
    if (lexeme == "xzr")
        return 31;
    if (lexeme[0] == 'x')
    {
        return std::stoi(lexeme.substr(1));
    }
    formatError("Invalid register: " + lexeme);
    return 0;
}

/** Takes a tokenization of an ARM64 assembly file as input, then outputs a list of parameters for compileLine,
 * replacing label uses with their respective addresses. Prints label addresses into standard out.
 *
 * If the file is not found, print an error and returns a non-0 value.
 *
 * @return 0 on success, non-0 on error
 */
int _main(int argc, char *argv[])
{
    if (argc > 2)
    {
        std::cerr << "Usage:" << std::endl
                  << "\ttokenasm [FILE]" << std::endl
                  << std::endl
                  << "If FILE is unspecified or if FILE is `-`, read tokenized assembly from standard "
                  << "in. Otherwise, read tokenized assembly from FILE." << std::endl;
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
        formatError((std::stringstream() << "File '" << argv[1] << "' not found!").str());
        return 1;
    }
    Token currToken;
    std::vector<Token> tokens;
    while (!in.eof())
    {
        in >> currToken;
        if (!in.fail())
        {
            if (currToken.type == NONE)
            {
                formatError("Invalid token type");
            }
            tokens.push_back(currToken);
        }
    }

    // -- YOUR CODE HERE --
    // You've been given a vector of all the tokens, so you're now free to manipulate and scan all tokens as many times as necessary.
    // Go ham!
    // Build symbol table
    std::map<std::string, int64_t> symTable;
    std::vector<std::string> label;
    int64_t current = 0;
    size_t i = 0;

    while (i < tokens.size())
    {
        if (tokens[i].type == NEWLINE)
        {
            i++;
            continue;
        }

        if (tokens[i].type == LABEL)
        {
            std::string labelName = tokens[i].lexeme;
            if (labelName.back() == ':')
            {
                labelName = labelName.substr(0, labelName.length() - 1);
            }
            symTable[labelName] = current;
            label.push_back(labelName);
            i++;
            if (i < tokens.size() && tokens[i].type != NEWLINE)
            {
                formatError("Must be followed by NEWLINE or be at end");
            }
            if (i < tokens.size())
            {
                i++;
            }
            continue;
        }

        if (i >= tokens.size())
        {
            break;
        }
        if (tokens[i].type == NEWLINE)
        {
            i++;
            continue;
        }

        TokenType type = tokens[i].type;

        if (type == DOTID)
        {
            if (tokens[i].lexeme == ".8byte")
            {
                i++;
                if (i >= tokens.size() || (tokens[i].type != HEXINT && tokens[i].type != INT && tokens[i].type != ID))
                {
                    formatError("Expected integer");
                }
                i++; // skip the integer
                current += 8;
                // Must be followed by NEWLINE or be at end
                if (i < tokens.size() && tokens[i].type != NEWLINE)
                {
                    formatError("Must be followed by NEWLINE or be at end");
                }
                if (i < tokens.size())
                    i++; // skip newline
            }
            else
            {
                formatError("Unknown directive: " + tokens[i].lexeme);
            }
        }
        else if (type == ID)
        {
            std::string instruction = tokens[i].lexeme;
            if (instruction.size() > 2 && instruction[0] == 'b' && instruction[1] == '.')
            {
                formatError("Conditional branch must be tokenized as ID b followed by DOTID .cond");
            }
            i++; // skip instruction
            if (instruction == "b" && i < tokens.size() && tokens[i].type == DOTID)
            {
                i++;
            }
            while (i < tokens.size() && tokens[i].type != NEWLINE)
            {
                i++;
            }
            current += 4;
            if (i < tokens.size())
                i++; // skip newline
        }
        else
        {
            formatError("Unexpected token: " + tokenTypeToString(type));
        }
    }

    // Output symbol table to stderr
    for (const auto &label : label)
    {
        std::cerr << label << " " << symTable[label] << "\n";
    }

    // Second pass: Generate machine code
    i = 0;
    current = 0;

    while (i < tokens.size())
    {
        if (tokens[i].type == NEWLINE)
        {
            i++;
            continue;
        }

        if (tokens[i].type == LABEL)
        {
            i++;
            if (i < tokens.size() && tokens[i].type != NEWLINE)
            {
                formatError("Must be followed by NEWLINE or be at end");
            }
            if (i < tokens.size())
            {
                i++;
            }
            continue;
        }

        if (i >= tokens.size())
        {
            break;
        }
        if (tokens[i].type == NEWLINE)
        {
            i++;
            continue;
        }

        TokenType type = tokens[i].type;

        if (type == DOTID)
        {
            if (tokens[i].lexeme == ".8byte")
            {
                i++;
                if (i >= tokens.size() || (tokens[i].type != HEXINT && tokens[i].type != INT && tokens[i].type != ID))
                {
                    formatError("Expected integer after .8byte");
                }
                int64_t value = 0;
                if (tokens[i].type == ID)
                {
                    const std::string &labelName = tokens[i].lexeme;
                    if (symTable.find(labelName) == symTable.end())
                    {
                        formatError("Undefined label: " + labelName);
                    }
                    value = symTable[labelName];
                }
                else
                {
                    value = parseInteger(tokens[i].lexeme);
                }
                i++;
                // Output 8 bytes in little-endian
                for (int j = 0; j < 8; ++j)
                {
                    std::cout << (char)((value >> (j * 8)) & 0xFF);
                }
                current += 8;
                if (i < tokens.size() && tokens[i].type == NEWLINE)
                    i++;
            }
            else
            {
                formatError("Unknown directive: " + tokens[i].lexeme);
            }
        }
        else if (type == ID)
        {
            std::string instruction = tokens[i].lexeme;
            if (instruction.size() > 2 && instruction[0] == 'b' && instruction[1] == '.')
            {
                formatError("Conditional branch must be tokenized as ID");
            }
            i++;

            if (instruction == "b" && i < tokens.size() && tokens[i].type == DOTID)
            {
                instruction += tokens[i].lexeme;
                i++;
            }

            std::vector<int> params;

            // Parse parameters
            while (i < tokens.size() && tokens[i].type != NEWLINE)
            {
                TokenType t = tokens[i].type;

                if (t == REG || t == ZREG)
                {
                    params.push_back(parseRegister(tokens[i].lexeme));
                    i++;
                }
                else if (t == ID && tokens[i].lexeme == "sp" && !instructionAllowsLabelOperand(instruction))
                {
                    params.push_back(31);
                    i++;
                }
                else if (t == INT || t == HEXINT)
                {
                    params.push_back(parseInteger(tokens[i].lexeme));
                    i++;
                }
                else if (t == ID)
                {
                    if (!instructionAllowsLabelOperand(instruction))
                    {
                        formatError("Unexpected token " + tokens[i].lexeme + " while processing " + instruction);
                    }
                    // Label reference
                    std::string labelName = tokens[i].lexeme;
                    if (symTable.find(labelName) == symTable.end())
                    {
                        formatError("Undefined label: " + labelName);
                    }
                    // Calculate offset from current instruction
                    int64_t offset = symTable[labelName] - current;
                    params.push_back(offset);
                    i++;
                }
                else if (t == COMMA)
                {
                    i++;
                }
                else if (t == LBRACK)
                {
                    i++;
                }
                else if (t == RBRACK)
                {
                    i++;
                }
                else
                {
                    formatError("Unexpected token : " + tokenTypeToString(t));
                }
            }

            // Call 
            
            uint32_t machineCode;
            int p1 = params.size() > 0 ? params[0] : 0;
            int p2 = params.size() > 1 ? params[1] : 0;
            int p3 = params.size() > 2 ? params[2] : 0;

            if (!compileLine(machineCode, instruction, p1, p2, p3))
            {
                return 1;
            }

            // Output machine code
            std::cout << (char)((machineCode >> 24) & 0xFF)
                      << (char)((machineCode >> 16) & 0xFF)
                      << (char)((machineCode >> 8) & 0xFF)
                      << (char)((machineCode >> 0) & 0xFF);

            current += 4;
            if (i < tokens.size() && tokens[i].type == NEWLINE)
                i++;
        }
        else
        {
            formatError("Unexpected token: " + tokenTypeToString(type));
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    try
    {
        _main(argc, argv);
        return 0;
    }
    catch (std::exception &e)
    {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}