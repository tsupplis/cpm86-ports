/*
 ** tinasm: NASM-compatible mini assembler for 8086
 ** tinasm modifications by pts@fazekas.hu at Mon Jun 20 10:22:13 CEST 2022
 **
 ** Based on Tinyasm (8086/8088 assembler for DOS) by Oscar Toledo G:
 ** https://github.com/nanochess/tinyasm/commit/2f213d809b685ccc5bccf52e2dbdd25dceedadd7
 ** Tinyasm Creation date: Oct/01/2019.
 ** tinasm.c is a concatenation of tinyasm.c and ins.c from the commit above.
 **
 ** The goal of tinasm is to provide critical bugfixes, portability
 ** improvements and deterministic cross-compilation over Tinyasm. For NASM
 ** compatibility improvements, memory usage optimizations, see mininasm.c in
 ** https://github.com/pts/mininasm instead.
 **
 ** This version is for CP/M-86 only, built with Aztec C (small model), see
 ** the Makefile. Aztec stdio is always binary and rejects "b" in the fopen
 ** mode. The instruction table is not in this file, see instr.txt.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cmdseg.h"


#define DEBUG

#define GET_INT16(value) (int)(sizeof(short) == 2 ? (short)(value) : (short)(((short)(value) & 0x7fff) | -((short)(value) & 0x8000U)))  /* Sign-extended. */
#define GET_UINT16(value) (unsigned)(sizeof(unsigned short) == 2 ? (unsigned short)(value) : (unsigned short)(value) & 0xffffU)  /* Zero-extended. */

unsigned line_number;

char *output_filename;
FILE *output;

char *listing_filename;
FILE *listing;

int assembler_step;
int default_start_address;
int start_address;
int address;
int first_time;
int cmd_format;
long code_size;

int instruction_addressing;
int instruction_offset;
int instruction_offset_width;

int instruction_register;

int instruction_value;
int instruction_value2;

#define MAX_SIZE        256

char line[MAX_SIZE];
char part[MAX_SIZE];
char name[MAX_SIZE];
char expr_name[MAX_SIZE];
char global_label[MAX_SIZE];
const char *prev_p;
const char *p;

char *g;
char generated[8];

unsigned errors;
unsigned warnings;
unsigned bytes;
int change;
int change_number;

struct label {
    struct label *left;
    struct label *right;
    int value;
    char name[1];
};

struct label *label_list;
struct label *last_label;
int undefined;
int matching;                   /* Set while trying instruction patterns. */
char undef_name[MAX_SIZE];      /* Last undefined label seen while matching. */

/*
 ** The instruction table is not compiled into the program (Aztec C limits
 ** the string space of a module). It is built by mkinstr.py from instr.txt
 ** and added to tinasm.cmd as the EXTRA group with cmdmod. Layout: word
 ** count, count words with the offset of each entry, then the entries
 ** (NAME 0 operands 0 encoding 0).
 */
#define INSTR_ENTRY 64      /* Max bytes of one entry, keep in sync with mkinstr.py */

cmdseg instr_seg;
unsigned instr_count;

const char *reg1[16] = {
    "AL",
    "CL",
    "DL",
    "BL",
    "AH",
    "CH",
    "DH",
    "BH",
    "AX",
    "CX",
    "DX",
    "BX",
    "SP",
    "BP",
    "SI",
    "DI"
};

void message(int error, const char *message);

const char *match_register(const char *p, int width, int *value);
const char *match_expression(const char *match_p);

/*
 ** Define a new label
 */
struct label *define_label(const char *name, int value) {
    struct label *label;
    struct label *explore;
    int c;
    
    /* Allocate label */
    label = (struct label*)malloc(sizeof(struct label) + strlen(name));
    if (label == 0) {
        fprintf(stderr, "Out of memory for label\r\n");
        exit(1);
        return 0;
    }
    
    /* Fill label */
    label->left = 0;
    label->right = 0;
    label->value = value;
    strcpy(label->name, name);
    
    /* Populate binary tree */
    if (label_list == 0) {
        label_list = label;
    } else {
        explore = label_list;
        while (1) {
            c = strcmp(label->name, explore->name);
            if (c < 0) {
                if (explore->left == 0) {
                    explore->left = label;
                    break;
                }
                explore = explore->left;
            } else if (c > 0) {
                if (explore->right == 0) {
                    explore->right = label;
                    break;
                }
                explore = explore->right;
            }
        }
    }
    return label;
}

/*
 ** Find a label
 */
struct label *find_label(const char *name) {
    struct label *explore;
    int c;
    
    /* Follows a binary tree */
    explore = label_list;
    while (explore != 0) {
        c = strcmp(name, explore->name);
        if (c == 0)
            return explore;
        if (c < 0)
            explore = explore->left;
        else
            explore = explore->right;
    }
    return 0;
}

/*
 ** Sort recursively labels (already done by binary tree)
 */
void sort_labels(struct label *node) {
    struct label *pre;
    /* Morris in-order traversal of binary tree: iterative (non-recursive,
     * so it uses O(1) stack), modifies the tree pointers temporarily, but
     * then restores them, runs in O(n) time.
     */
    while (node) {
        if (!node->left) goto do_print;
        for (pre = node->left; pre->right && pre->right != node; pre = pre->right) {}
        if (!pre->right) {
            pre->right = node;
            node = node->left;
        } else {
            pre->right = 0;
          do_print:
            fprintf(listing, "%-20s %04x\r\n", node->name, GET_UINT16(node->value));
            node = node->right;
        }
    }
}

/*
 ** Avoid spaces in input
 */
const char *avoid_spaces(const char *p) {
    while (isspace(*p))
        p++;
    return p;
}

/*
 ** Match addressing
 */
const char *match_addressing(const char *p, int width) {
    int reg;
    int reg2;
    const char *p2;
    int *bits;
    
    bits = &instruction_addressing;
    instruction_offset = 0;
    instruction_offset_width = 0;
    
    p = avoid_spaces(p);
    if (*p == '[') {
        p = avoid_spaces(p + 1);
        p2 = match_register(p, 16, &reg);
        if (p2 != 0) {
            p = avoid_spaces(p2);
            if (*p == ']') {
                p++;
                if (reg == 3) {   /* BX */
                    *bits = 0x07;
                } else if (reg == 5) {  /* BP */
                    *bits = 0x46;
                    instruction_offset = 0;
                    instruction_offset_width = 1;
                } else if (reg == 6) {  /* SI */
                    *bits = 0x04;
                } else if (reg == 7) {  /* DI */
                    *bits = 0x05;
                } else {    /* Not valid */
                    return 0;
                }
            } else if (*p == '+' || *p == '-') {
                if (*p == '+') {
                    p = avoid_spaces(p + 1);
                    p2 = match_register(p, 16, &reg2);
                } else {
                    p2 = 0;
                }
                if (p2 != 0) {
                    if ((reg == 3 && reg2 == 6) || (reg == 6 && reg2 == 3)) {   /* BX+SI / SI+BX */
                        *bits = 0x00;
                    } else if ((reg == 3 && reg2 == 7) || (reg == 7 && reg2 == 3)) {    /* BX+DI / DI+BX */
                        *bits = 0x01;
                    } else if ((reg == 5 && reg2 == 6) || (reg == 6 && reg2 == 5)) {    /* BP+SI / SI+BP */
                        *bits = 0x02;
                    } else if ((reg == 5 && reg2 == 7) || (reg == 7 && reg2 == 5)) {    /* BP+DI / DI+BP */
                        *bits = 0x03;
                    } else {    /* Not valid */
                        return 0;
                    }
                    p = avoid_spaces(p2);
                    if (*p == ']') {
                        p++;
                    } else if (*p == '+' || *p == '-') {
                        p2 = match_expression(p);
                        if (p2 == 0)
                            return 0;
                        instruction_offset = instruction_value;
                        p = avoid_spaces(p2);
                        if (*p != ']')
                            return 0;
                        p++;
                        if (instruction_offset >= -0x80 && instruction_offset <= 0x7f) {
                            instruction_offset_width = 1;
                            *bits |= 0x40;
                        } else {
                            instruction_offset_width = 2;
                            *bits |= 0x80;
                        }
                    } else {    /* Syntax error */
                        return 0;
                    }
                } else {
                    if (reg == 3) {   /* BX */
                        *bits = 0x07;
                    } else if (reg == 5) {  /* BP */
                        *bits = 0x06;
                    } else if (reg == 6) {  /* SI */
                        *bits = 0x04;
                    } else if (reg == 7) {  /* DI */
                        *bits = 0x05;
                    } else {    /* Not valid */
                        return 0;
                    }
                    p2 = match_expression(p);
                    if (p2 == 0)
                        return 0;
                    instruction_offset = instruction_value;
                    p = avoid_spaces(p2);
                    if (*p != ']')
                        return 0;
                    p++;
                    if (instruction_offset >= -0x80 && instruction_offset <= 0x7f) {
                        instruction_offset_width = 1;
                        *bits |= 0x40;
                    } else {
                        instruction_offset_width = 2;
                        *bits |= 0x80;
                    }
                }
            } else {    /* Syntax error */
                return 0;
            }
        } else {    /* No valid register, try expression (absolute addressing) */
            p2 = match_expression(p);
            if (p2 == 0)
                return 0;
            instruction_offset = instruction_value;
            p = avoid_spaces(p2);
            if (*p != ']')
                return 0;
            p++;
            *bits = 0x06;
            instruction_offset_width = 2;
        }
    } else {    /* Register */
        p = match_register(p, width, &reg);
        if (p == 0)
            return 0;
        *bits = 0xc0 | reg;
    }
    return p;
}

/*
 ** Check for a label character
 */
int islabel(int c) {
    return isalpha(c) || isdigit(c) || c == '_' || c == '.';
}

void inc_u16_capped(unsigned *p) {
    ++*p;
    if (GET_INT16(*p) == 0) --*p;  /* Capped at 0xffff. */
}

#define MAX_ERRORS 128

/*
 ** Count an error. After MAX_ERRORS of them give up: close and delete the
 ** output and listing files and stop.
 */
void count_error(void) {
    inc_u16_capped(&errors);
    if (errors >= MAX_ERRORS) {
        fprintf(stderr, "Error: too many errors (%d), aborting\r\n", MAX_ERRORS);
        if (output != 0)
            fclose(output);
        if (listing != 0)
            fclose(listing);
        if (output_filename != 0)
            remove(output_filename);
        if (listing_filename != 0)
            remove(listing_filename);
        exit(1);
    }
}

/*
 ** Match register
 */
const char *match_register(const char *p, int width, int *value) {
    char reg[3];
    int c;
    
    p = avoid_spaces(p);
    if (!isalpha(p[0]) || !isalpha(p[1]) || islabel(p[2]))
        return 0;
    reg[0] = p[0];
    reg[1] = p[1];
    reg[2] = '\0';
    if (width == 8) {   /* 8-bit */
        for (c = 0; c < 8; c++)
            if (strcmp(reg, reg1[c]) == 0)
                break;
        if (c < 8) {
            *value = c;
            return p + 2;
        }
    } else {    /* 16-bit */
        for (c = 0; c < 8; c++)
            if (strcmp(reg, reg1[c + 8]) == 0)
                break;
        if (c < 8) {
            *value = c;
            return p + 2;
        }
    }
    return 0;
}

/*
 ** Read character for string or character literal
 **
 ** This is incompatible with NASM (any version 0.98.39 and 2.13.2) and Yasm
 ** (1.2.0 and 1.3.0). These assemblers don't process backslashes in '...'
 ** or "..." string literals.
 */
const char *read_character(const char *p, int *c) {
    if (*p == '\\') {
        p++;
        if (*p == '\'') {
            *c = '\'';
            p++;
        } else if (*p == '\"') {
            *c = '"';
            p++;
        } else if (*p == '\\') {
            *c = '\\';
            p++;
        } else if (*p == 'a') {
            *c = 0x07;
            p++;
        } else if (*p == 'b') {
            *c = 0x08;
            p++;
        } else if (*p == 't') {
            *c = 0x09;
            p++;
        } else if (*p == 'n') {
            *c = 0x0a;
            p++;
        } else if (*p == 'v') {
            *c = 0x0b;
            p++;
        } else if (*p == 'f') {
            *c = 0x0c;
            p++;
        } else if (*p == 'r') {
            *c = 0x0d;
            p++;
        } else if (*p == 'e') {
            *c = 0x1b;
            p++;
        } else if (*p >= '0' && *p <= '7') {
            *c = 0;
            while (*p >= '0' && *p <= '7') {
                *c = *c * 8 + (*p - '0');
                p++;
            }
        } else {
            p--;
            message(1, "bad escape inside string");
        }
    } else {
        *c = *p;
        p++;
    }
    return p;
}

#ifndef CONFIG_MATCH_STACK_DEPTH
#define CONFIG_MATCH_STACK_DEPTH 100
#endif

/*
 ** Match expression.
 ** Saves the result to `instruction_value'.
 */
/*
 ** Return nonzero if p starts a hexadecimal literal with an h suffix,
 ** for example 0B8h or 1234h. It must start with a digit.
 */
int is_hex_suffix(const char *p) {
    if (!isdigit(*p))
        return 0;
    while (isxdigit(*p))
        p++;
    return tolower(*p) == 'h' && !isalnum(p[1]) && p[1] != '_';
}

const char *match_expression(const char *match_p) {
    static struct match_stack_item {
        signed char casei;
        unsigned char level;
        int value1;
    } match_stack[CONFIG_MATCH_STACK_DEPTH];  /* This static variable makes match_expression(...) not reentrant. */
    struct match_stack_item *msp;  /* Stack pointer within match_stack. */
    int value1;
    int value2;
    char *p2;
    struct label *label;
    char c;
    unsigned char level;

    level = 0;
    msp = match_stack;
    goto do_match;
  do_pop:
    --msp;
    value2 = value1;
    value1 = msp->value1;
    level = msp->level;
    if (msp->casei < 0) {  /* End of expression in patentheses. */
        value1 = value2;
        match_p = avoid_spaces(match_p);
        if (match_p[0] != ')') {
            message(1, "Missing close paren");
          match_error:
            instruction_value = 0;
            return 0;
        }
        match_p++;
        if (++msp->casei != 0) {
            level = 0;
            if (++msp == match_stack + sizeof(match_stack) / sizeof(match_stack[0])) goto too_deep;
        }
        goto have_value1;
    }
#define MATCH_CASEI_LEVEL_TO_VALUE2(casei2, level2) do { msp->casei = casei2; msp->level = level; level = level2; goto do_push; case casei2: ; } while (0)
    switch (msp->casei) {  /* This will jump after one of the MATCH_CASEI_LEVEL_TO_VALUE2(...) macros. */
      do_push:
        msp->value1 = value1;
        if (++msp == match_stack + sizeof(match_stack) / sizeof(match_stack[0])) { too_deep:
            message(1, "Expression too deep");  /* Stack overflow in match stack. */
            goto match_error;
        }
      do_match:
        match_p = avoid_spaces(match_p);
        value1 = 0;  /* In addition to preventing duplicate initialization below, it also does pacify GCC 7.5.0: do_push jumped to by MATCH_CASEI_LEVEL_TO_VALUE2 does an `msp->value1 = value1'. */
        if ((c = match_p[0]) == '(') {  /* Parenthesized expression. */
            /* Count the consecutive open parentheses, and add a single match_stack_item. */
            for (; (c = (match_p = avoid_spaces(match_p))[0]) == '(' && value1 > -127; ++match_p, --value1) {}
            msp->casei = value1; msp->level = level; level = 0; goto do_push;
        } else if (c == '-' || c == '+') {  /* Unary - and -. */
            for (;;) {  /* Shortcut to squeeze multiple unary - and + operators to a single match_stack_item. */
                match_p = avoid_spaces(match_p + 1);
                if (match_p[0] == '+') {}
                else if (match_p[0] == '-') { c ^= 6; }  /* Switch between ASCII '+' and '-'. */
                else { break; }
            }
            if (c == '-') {
              MATCH_CASEI_LEVEL_TO_VALUE2(2, 6);
              value1 -= value2;
            } else {
              MATCH_CASEI_LEVEL_TO_VALUE2(3, 6);
              value1 += value2;
            }
        } else if (is_hex_suffix(match_p)) {  /* Hexadecimal with h suffix */
            for (; c = match_p[0], isxdigit(c); ++match_p) {
                c -= '0';
                if ((unsigned char)c > 9) c = (c & ~32) - 7;
                value1 = (value1 << 4) | c;
            }
            ++match_p;  /* Skip the h. */
        } else if (c == '0' && tolower(match_p[1]) == 'b') {  /* Binary */
            match_p += 2;
            /*value1 = 0;*/
            while (match_p[0] == '0' || match_p[0] == '1' || match_p[0] == '_') {
                if (match_p[0] != '_') {
                    value1 <<= 1;
                    if (match_p[0] == '1')
                        value1 |= 1;
                }
                match_p++;
            }
        } else if (c == '0' && tolower(match_p[1]) == 'x') {  /* Hexadecimal */
            match_p += 2;
          parse_hex:
            /*value1 = 0;*/
            for (; c = match_p[0], isxdigit(c); ++match_p) {
                c -= '0';
                if ((unsigned char)c > 9) c = (c & ~32) - 7;
                value1 = (value1 << 4) | c;
            }
        } else if (c == '$' && isdigit(match_p[1])) {  /* Hexadecimal */
            /* This is nasm syntax, notice no letter is allowed after $ */
            /* So it's preferrable to use prefix 0x for hexadecimal */
            match_p += 1;
            goto parse_hex;
        } else if (c == '\'') {  /* Character constant */
            match_p = read_character(match_p + 1, &value1);
            if (match_p[0] != '\'') {
                message(1, "Missing apostrophe");
                goto match_error;
            } else {
                ++match_p;
            }
        } else if (isdigit(c)) {   /* Decimal */
            /*value1 = 0;*/
            for (; (unsigned char)(c = match_p[0] - '0') <= 9; ++match_p) {
                value1 = value1 * 10 + c;
            }
        } else if (c == '$' && match_p[1] == '$') { /* Start address */
            match_p += 2;
            value1 = start_address;
        } else if (c == '$') { /* Current address */
            match_p++;
            value1 = address;
        } else if (isalpha(c) || c == '_' || c == '.') {  /* Start of label. */
            p2 = expr_name;
            if (c == '.') {
                strcpy(expr_name, global_label);
                while (*p2 != '\0')
                    p2++;
            }
            while (isalpha(match_p[0]) || isdigit(match_p[0]) || match_p[0] == '_' || match_p[0] == '.')
                *p2++ = *match_p++;
            *p2 = '\0';
            for (c = 0; c < 16; c++) {
                if (strcmp(expr_name, reg1[(unsigned char)c]) == 0) goto match_error;    /* Using a register name as a label is an error. */
            }
            label = find_label(expr_name);
            if (label == 0) {
                /*value1 = 0;*/
                undefined++;
                if (assembler_step == 2) {
                    if (matching) {
                        /* Only trying a pattern, it may not be the right one. */
                        strcpy(undef_name, expr_name);
                    } else {
                        fprintf(stderr, "Error: undefined label '%s' at line %u\r\n", expr_name, line_number);
                        count_error();
                    }
                }
            } else {
                value1 = label->value;
            }
        } else {
            /* TODO(pts): Make this match syntax error nonsilent? What about when trying instructions? */
            goto match_error;
        }
        /* Now value1 contains the value of the expression parsed so far. */
      have_value1:
        if (level <= 5) {
            while (1) {
                match_p = avoid_spaces(match_p);
                if ((c = match_p[0]) == '*') {  /* Multiply operator. */  
                    match_p++;
                    MATCH_CASEI_LEVEL_TO_VALUE2(10, 6);
                    value1 *= value2;
                } else if (c == '/') {  /* Division operator. */
                    match_p++;
                    MATCH_CASEI_LEVEL_TO_VALUE2(11, 6);
                    if (GET_UINT16(value2) == 0) {
                        if (assembler_step == 2)
                            message(1, "division by zero");
                        value2 = 1;
                    }
                    value1 = GET_UINT16(value1) / GET_UINT16(value2);
                } else if (c == '%') {  /* Modulo operator. */
                    match_p++;
                    MATCH_CASEI_LEVEL_TO_VALUE2(12, 6);
                    if (GET_UINT16(value2) == 0) {
                        if (assembler_step == 2)
                            message(1, "modulo by zero");
                        value2 = 1;
                    }
                    /* Since '/' uses unsigned division, '%' must also use it,
                     * otherwise this wouldn't hold: (a % b) == a - (a / b) * b.
                     */
                    value1 = GET_UINT16(value1) % GET_UINT16(value2);
                } else {
                    break;
                }
            }
        }
        if (level <= 4) {
            while (1) {
                match_p = avoid_spaces(match_p);
                if ((c = match_p[0]) == '+') {  /* Add operator. */
                    match_p++;
                    MATCH_CASEI_LEVEL_TO_VALUE2(13, 5);
                    value1 += value2;
                } else if (c == '-') {  /* Subtract operator. */
                    match_p++;
                    MATCH_CASEI_LEVEL_TO_VALUE2(14, 5);
                    value1 -= value2;
                } else {
                    break;
                }
            }
        }
        if (level <= 3) {
            while (1) {
                match_p = avoid_spaces(match_p);
                if (((c = match_p[0]) == '<' && match_p[1] == '<') || (c == '>' && match_p[1] == '>')) { /* Shift to left */
                    match_p += 2;
                    if (c == '<') {
                        MATCH_CASEI_LEVEL_TO_VALUE2(15, 4);
                        c = 1;
                    } else {
                        MATCH_CASEI_LEVEL_TO_VALUE2(16, 4);
                        c = 0;
                    }
                    value2 = GET_INT16(value2);
                    if (value2 < 0) {
                        message(1, "shift by negative");
                        return 0;
                    }
                    /* Checking (value2 > 15) to avoid i386 quirk: (x << 32) == x. */
                    value1 = c ? ((value2 > 15) ? 0 : value1 << value2) : (GET_INT16(value1) >> ((value2 > 15) ? 15 : value2));
                } else {
                    break;
                }
            }
        }
        if (level <= 2) {
            while (1) {
                match_p = avoid_spaces(match_p);
                if (match_p[0] == '&') {    /* Binary AND */
                    match_p++;
                    MATCH_CASEI_LEVEL_TO_VALUE2(17, 3);
                    value1 &= value2;
                } else {
                    break;
                }
            }
        }
        if (level <= 1) {
            while (1) {
                match_p = avoid_spaces(match_p);
                if (match_p[0] == '^') {    /* Binary XOR */
                    match_p++;
                    MATCH_CASEI_LEVEL_TO_VALUE2(18, 2);
                    value1 ^= value2;
                } else {
                    break;
                }
            }
        }
        if (level == 0) {  /* Top tier. */
            while (1) {
                match_p = avoid_spaces(match_p);
                if (match_p[0] == '|') {    /* Binary OR */
                    match_p++;
                    MATCH_CASEI_LEVEL_TO_VALUE2(19, 1);
                    value1 |= value2;
                } else {
                    break;
                }
            }
        }
    }
    if (msp != match_stack) goto do_pop;
    instruction_value = value1;
    return avoid_spaces(match_p);
}

/*
 ** Emit one byte to output
 */
void emit_byte(int byte) {
    char buf[1];
    
    if (assembler_step == 2) {
        if (g != 0 && g < generated + sizeof(generated))
            *g++ = byte;
        buf[0] = byte;
        /* Cannot use fputc because DeSmet C expands to CR LF */
        fwrite(buf, 1, 1, output);
        bytes++;
    }
    address++;
}

/*
 ** CP/M-86 CMD output, 8080 model, same layout as bin2cmd: 128-byte header
 ** with one relocatable code group, a 256-byte zero page, the code padded
 ** to a paragraph, then zeros up to a 512-byte record. The code size is
 ** the one found by the previous pass.
 */
void cmd_header(void) {
    static char hdr[128 + 256];
    long paras;
    
    paras = (code_size + 15) / 16 + 16;
    hdr[0] = 1;                         /* Code group */
    hdr[1] = paras & 255;               /* Length in paragraphs */
    hdr[2] = (paras >> 8) & 255;
    hdr[5] = hdr[1];                    /* Min size, same as length */
    hdr[6] = hdr[2];
    fwrite(hdr, 1, sizeof(hdr), output);
}

void cmd_trailer(void) {
    static char zeros[512];
    long total;
    unsigned n;
    
    n = (16 - (code_size % 16)) % 16;
    total = 128 + 256 + code_size + n;
    fwrite(zeros, 1, n, output);
    n = (512 - (total % 512)) % 512;
    fwrite(zeros, 1, n, output);
}

/*
 ** Search for a match with instruction
 */
const char *match(const char *p, const char *pattern, const char *decode) {
    const char *p2;
    int c;
    int d;
    int bit;
    int qualifier;
    const char *base;
    
    undefined = 0;
    while (*pattern) {
/*        fputc(*pattern, stdout);*/
        if (*pattern == '%') {	/* Special */
            pattern++;
            if (*pattern == 'd') {  /* Addressing */
                pattern++;
                qualifier = 0;
                if (memcmp(p, "WORD", 4) == 0 && !isalpha(p[4])) {
                    p = avoid_spaces(p + 4);
                    if (*p != '[')
                        return 0;
                    qualifier = 16;
                } else if (memcmp(p, "BYTE", 4) == 0 && !isalpha(p[4])) {
                    p = avoid_spaces(p + 4);
                    if (*p != '[')
                        return 0;
                    qualifier = 8;
                }
                if (*pattern == 'w') {
                    pattern++;
                    if (qualifier != 16 && match_register(p, 16, &d) == 0)
                        return 0;
                } else if (*pattern == 'b') {
                    pattern++;
                    if (qualifier != 8 && match_register(p, 8, &d) == 0)
                        return 0;
                } else {
                    if (qualifier == 8 && *pattern != '8')
                        return 0;
                    if (qualifier == 16 && *pattern != '1')
                        return 0;
                }
                if (*pattern == '8') {
                    pattern++;
                    p2 = match_addressing(p, 8);
                    if (p2 == 0)
                        return 0;
                    p = p2;
                } else if (*pattern == '1' && pattern[1] == '6') {
                    pattern += 2;
                    p2 = match_addressing(p, 16);
                    if (p2 == 0)
                        return 0;
                    p = p2;
                } else {
                    return 0;
                }
            } else if (*pattern == 'r') {   /* Register */
                pattern++;
                if (*pattern == '8') {
                    pattern++;
                    p2 = match_register(p, 8, &instruction_register);
                    if (p2 == 0)
                        return 0;
                    p = p2;
                } else if (*pattern == '1' && pattern[1] == '6') {
                    pattern += 2;
                    p2 = match_register(p, 16, &instruction_register);
                    if (p2 == 0)
                        return 0;
                    p = p2;
                } else {
                    return 0;
                }
            } else if (*pattern == 'i') {   /* Immediate */
                pattern++;
                if (*pattern == '8') {
                    pattern++;
                    p2 = match_expression(p);
                    if (p2 == 0)
                        return 0;
                    p = p2;
                } else if (*pattern == '1' && pattern[1] == '6') {
                    pattern += 2;
                    p2 = match_expression(p);
                    if (p2 == 0)
                        return 0;
                    p = p2;
                } else {
                    return 0;
                }
            } else if (*pattern == 'a') {   /* Address for jump */
                pattern++;
                if (*pattern == '8') {
                    pattern++;
                    p = avoid_spaces(p);
                    qualifier = 0;
                    if (memcmp(p, "SHORT", 5) == 0 && isspace(p[5])) {
                        p += 5;
                        qualifier = 1;
                    }
                    p2 = match_expression(p);
                    if (p2 == 0)
                        return 0;
                    if (qualifier == 0) {
                        c = instruction_value - (address + 2);
                        if (undefined == 0 && (c < -128 || c > 127) && memcmp(decode, "xeb", 3) == 0)
                            return 0;
                    }
                    p = p2;
                } else if (*pattern == '1' && pattern[1] == '6') {
                    pattern += 2;
                    p = avoid_spaces(p);
                    if (memcmp(p, "SHORT", 5) == 0 && isspace(p[5]))
                        p2 = 0;
                    else
                        p2 = match_expression(p);
                    if (p2 == 0)
                        return 0;
                    p = p2;
                } else {
                    return 0;
                }
            } else if (*pattern == 's') {   /* Signed immediate */
                pattern++;
                if (*pattern == '8') {
                    pattern++;
                    p = avoid_spaces(p);
                    qualifier = 0;
                    if (memcmp(p, "BYTE", 4) == 0 && isspace(p[4])) {
                        p += 4;
                        qualifier = 1;
                    }
                    p2 = match_expression(p);
                    if (p2 == 0)
                        return 0;
                    if (qualifier == 0) {
                        c = instruction_value;
                        if (undefined != 0)
                            return 0;
                        if (undefined == 0 && (c < -128 || c > 127))
                            return 0;
                    }
                    p = p2;
                } else {
                    return 0;
                }
            } else if (*pattern == 'f') {   /* FAR pointer */
                pattern++;
                if (memcmp(p, "SHORT", 5) == 0 && isspace(p[5])) {
                    return 0;
                 } else if (*pattern == '3' && pattern[1] == '2') {
                    pattern += 2;
                    p2 = match_expression(p);
                    if (p2 == 0)
                        return 0;
                    instruction_value2 = instruction_value;
                    if (*p2 != ':')
                        return 0;
                    p = p2 + 1;
                    p2 = match_expression(p);
                    if (p2 == 0)
                        return 0;
                    p = p2;
                } else {
                    return 0;
                }
            } else {
                return 0;
            }
            continue;
        }
        if (toupper(*p) != *pattern)
            return 0;
        p++;
        if (*pattern == ',')    /* Allow spaces after comma */
            p = avoid_spaces(p);
        pattern++;
    }

    /*
     ** Instruction properly matched, now generate binary
     */
    base = decode;
    while (*decode) {
        decode = avoid_spaces(decode);
        if (decode[0] == 'x') { /* Byte */
            c = toupper(decode[1]);
            c -= '0';
            if (c > 9)
                c -= 7;
            d = toupper(decode[2]);
            d -= '0';
            if (d > 9)
                d -= 7;
            c = (c << 4) | d;
            emit_byte(c);
            decode += 3;
        } else {    /* Binary */
            if (*decode == 'b')
                decode++;
            bit = 0;
            c = 0;
            d = 0;
            while (bit < 8) {
                if (decode[0] == '0') { /* Zero */
                    decode++;
                    bit++;
                } else if (decode[0] == '1') {  /* One */
                    c |= 0x80 >> bit;
                    decode++;
                    bit++;
                } else if (decode[0] == '%') {  /* Special */
                    decode++;
                    if (decode[0] == 'r') { /* Register field */
                        decode++;
                        if (decode[0] == '8')
                            decode++;
                        else if (decode[0] == '1' && decode[1] == '6')
                            decode += 2;
                        c |= instruction_register << (5 - bit);
                        bit += 3;
                    } else if (decode[0] == 'd') {  /* Addressing field */
                        if (decode[1] == '8')
                            decode += 2;
                        else
                            decode += 3;
                        if (bit == 0) {
                            c |= instruction_addressing & 0xc0;
                            bit += 2;
                        } else {
                            c |= instruction_addressing & 0x07;
                            bit += 3;
                            d = 1;
                        }
                    } else if (decode[0] == 'i' || decode[0] == 's') {
                        if (decode[1] == '8') {
                            decode += 2;
                            c = instruction_value;
                            break;
                        } else {
                            decode += 3;
                            c = instruction_value;
                            instruction_offset = instruction_value >> 8;
                            instruction_offset_width = 1;
                            d = 1;
                            break;
                        }
                    } else if (decode[0] == 'a') {
                        if (decode[1] == '8') {
                            decode += 2;
                            c = instruction_value - (address + 1);
                            break;
                        } else {
                            decode += 3;
                            c = instruction_value - (address + 2);
                            instruction_offset = c >> 8;
                            instruction_offset_width = 1;
                            d = 1;
                            break;
                        }
                    } else if (decode[0] == 'f') {
                        decode += 3;
                        emit_byte(instruction_value);
                        c = instruction_value >> 8;
                        instruction_offset = instruction_value2;
                        instruction_offset_width = 2;
                        d = 1;
                        break;
                    } else {
                        fprintf(stderr, "decode: internal error 2\r\n");
                    }
                } else {
                    fprintf(stderr, "decode: internal error 1 (%s)\r\n", base);
                    break;
                }
            }
            emit_byte(c);
            if (d == 1) {
                d = 0;
                if (instruction_offset_width >= 1) {
                    emit_byte(instruction_offset);
                }
                if (instruction_offset_width >= 2) {
                    emit_byte(instruction_offset >> 8);
                }
            }
        }
    }
    return p;
}

/*
 ** Make a string lowercase
 */
void to_lowercase(char *p) {
    while (*p) {
        *p = tolower(*p);
        p++;
    }
}

/*
 ** Separate a portion of entry up to the first space
 */
void separate(void) {
    char *p2;
    
    while (*p && isspace(*p))
        p++;
    prev_p = p;
    p2 = part;
    while (*p && !isspace(*p) && *p != ';')
        *p2++ = *p++;
    *p2 = '\0';
    while (*p && isspace(*p))
        p++;
}

/*
 ** Check for end of line
 */
void check_end(const char *p) {
    p = avoid_spaces(p);
    if (*p && *p != ';') {
        fprintf(stderr, "Error: extra characters at end of line %d\r\n", line_number);
        count_error();
    }
}

/*
 ** Generate a message
 */
void message(int error, const char *message) {
    if (error) {
        fprintf(stderr, "Error: %s at line %d\r\n", message, line_number);
        count_error();
    } else {
        fprintf(stderr, "Warning: %s at line %d\r\n", message, line_number);
        ++warnings;
        if (GET_INT16(warnings) == 0) ++warnings;
    }
    if (listing != 0) {
        if (error) {
            fprintf(listing, "Error: %s at line %d\r\n", message, line_number);
        } else {
            fprintf(listing, "Warning: %s at line %d\r\n", message, line_number);
        }
    }
}

/*
 ** Open the instruction table (EXTRA group of the .cmd file)
 */
void instr_open(void) {
    unsigned n;
    
    if (cmdseg_open(&instr_seg, CMD_EXTRA, CMD_RDONLY) < 0
        || cmdseg_read(&instr_seg, &n, 2) != 2) {
        fprintf(stderr, "Error: instruction table (EXTRA group) not found, run cmdmod\r\n");
        exit(1);
    }
    instr_count = n;
}

/*
 ** Copy entry c of the instruction table to buf (INSTR_ENTRY bytes).
 ** Returns 0 if c is past the last entry.
 */
int instr_get(int c, char *buf) {
    unsigned off;
    
    if ((unsigned) c >= instr_count)
        return 0;
    if (cmdseg_seek(&instr_seg, 2UL + 2UL * c) < 0
        || cmdseg_read(&instr_seg, &off, 2) != 2
        || cmdseg_seek(&instr_seg, (unsigned long) off) < 0
        || cmdseg_read(&instr_seg, buf, INSTR_ENTRY) <= 0)
        return 0;
    buf[INSTR_ENTRY - 1] = 0;
    return 1;
}

/*
 ** Process an instruction
 */
void process_instruction() {
    const char *p2 = 0;
    const char *p3;
    char entry[INSTR_ENTRY];
    int c;
    int q;
    
    if (strcmp(part, "DB") == 0) {  /* Define byte */
        while (1) {
            p = avoid_spaces(p);
            if (*p == '"' || *p == '\'') {    /* ASCII text */
                q = *p++;
                while (*p && *p != q) {
                    p = read_character(p, &c);
                    emit_byte(c);
                }
                if (*p) {
                    p++;
                } else {
                    fprintf(stderr, "Error: unterminated string at line %d\r\n", line_number);
                }
            } else {
                p2 = match_expression(p);
                if (p2 == 0) {
                    fprintf(stderr, "Error: bad expression at line %d\r\n", line_number);
                    break;
                }
                emit_byte(instruction_value);
                p = p2;
            }
            p = avoid_spaces(p);
            if (*p == ',') {
                p++;
                continue;
            }
            check_end(p);
            break;
        }
        return;
    }
    if (strcmp(part, "DW") == 0) {  /* Define word */
        while (1) {
            p2 = match_expression(p);
            if (p2 == 0) {
                fprintf(stderr, "Error: bad expression at line %d\r\n", line_number);
                break;
            }
            emit_byte(instruction_value);
            emit_byte(instruction_value >> 8);
            p = avoid_spaces(p2);
            if (*p == ',') {
                p++;
                continue;
            }
            check_end(p);
            break;
        }
        return;
    }
    while (part[0]) {   /* Match against instruction set */
        c = 0;
        while (instr_get(c, entry)) {
            if (strcmp(part, entry) == 0) {
                p2 = entry;
                while (*p2++) ;
                p3 = p2;
                while (*p3++) ;
                
                matching = 1;
                p2 = match(p, p2, p3);
                matching = 0;
                if (p2 != 0 && assembler_step == 2 && undefined) {
                    fprintf(stderr, "Error: undefined label '%s' at line %u\r\n", undef_name, line_number);
                    count_error();
                }
                if (p2 != 0) {
                    p = p2;
                    break;
                }
            }
            c++;
        }
        if (c >= instr_count) {
            char m[25 + MAX_SIZE];
            
            sprintf(m, "Undefined instruction '%s %s'", part, p);
            message(1, m);
            break;
        } else {
            p = p2;
            separate();
        }
    }
}

/*
 ** Reset current address.
 ** Called anytime the assembler needs to generate code.
 */
void reset_address() {
    address = start_address = default_start_address;
}

/*
 ** Include a binary file
 */
void incbin(const char *fname) {
    FILE *input;
    static char buf[512];
    int size;
    int i;
    
    input = fopen(fname, "r");
    if (input == 0) {
        sprintf(buf, "Error: Cannot open '%s' for input", fname);
        message(1, buf);
        return;
    }
    
    while ((size = fread(buf, 1, sizeof(buf), input)) > 0) {
        for (i = 0; i < size; i++) {
            emit_byte(buf[i]);
        }
    }
    
    fclose(input);
}


/*
 ** Return nonzero if the text at p starts with DB, DW, EQU or TIMES as a
 ** whole word, so the word before it is a label without colon ("msg db 0").
 */
int is_data_word(const char *p) {
    int n;
    
    if (strncmp(p, "DB", 2) == 0 || strncmp(p, "DW", 2) == 0)
        n = 2;
    else if (strncmp(p, "EQU", 3) == 0)
        n = 3;
    else if (strncmp(p, "TIMES", 5) == 0)
        n = 5;
    else
        return 0;
    return p[n] == '\0' || p[n] == ';' || isspace(p[n]);
}

/*
 ** Do an assembler step
 */
void do_assembly(const char *fname) {
    long ofs;
    static FILE *input;  /* Using a global variable to prevent the allocation of read buffers (by fread) for each pending %INCLUDE. */
    static char include_buf[512];  /* Contains filenames. */
    static char *ibp;
    const char *p2;
    const char *p3;
    unsigned level;
    unsigned avoid_level;
    int times;
    int base;
    int include;
    int align;

    global_label[0] = '\0';
    base = 0;
    if (ibp == 0) ibp = include_buf;
  do_open:
    input = fopen(fname, "r");
    if (input == 0) { open_error:
        fprintf(stderr, "Error: cannot open '%s' for input\r\n", fname);
      io_error:
        count_error();
        goto do_return;
    }
    times = strlen(fname);
    if (times + (int)(2 + sizeof(level) + sizeof(avoid_level) + sizeof(line_number) + sizeof(ofs)) > include_buf + sizeof(include_buf) - ibp) {
        fclose(input);
        message(1, "assembly stack overflow, too many pending %INCLUDE files");
        goto io_error;
    }
    *ibp++ = '\0';  /* Sentinel to find benginning of the filename after an %INCLUDE has finished. */
    strcpy(ibp, fname);
    /*fname = ibp;*/
    ibp += times + 1;
    level = 1;
    avoid_level = 0;
    line_number = 0;
  do_assemble:
    while (fgets(line, sizeof(line), input)) {
        inc_u16_capped(&line_number);
        p = line;
        while (*p) {
            if (*p == '\'' && *(p - 1) != '\\') {
                p++;
                while (*p && *p != '\'' && *(p - 1) != '\\')
                    p++;
            } else if (*p == '"' && *(p - 1) != '\\') {
                p++;
                while (*p && *p != '"' && *(p - 1) != '\\')
                    p++;
            } else if (*p == ';') {
                while (*p)
                    p++;
                break;
            }
            *(char*)p = toupper(*p);
            p++;
        }
        if (p > (const char *)line && *(p - 1) == '\n')
            p--;
        if (p > (const char *)line && *(p - 1) == '\r')
            p--;
        *(char*)p = '\0';

        base = address;
        g = generated;
        include = 0;

        while (1) {
            p = line;
            separate();
            if (part[0] == '\0' && (*p == '\0' || *p == ';'))    /* Empty line */
                break;
            if (part[0] != '\0' && part[strlen(part) - 1] != ':' && is_data_word(p))
                strcat(part, ":");  /* Label without colon. */
            if (part[0] != '\0' && part[strlen(part) - 1] == ':') {	/* Label */
                part[strlen(part) - 1] = '\0';
                if (part[0] == '.') {
                    strcpy(name, global_label);
                    strcat(name, part);
                } else {
                    strcpy(name, part);
                    strcpy(global_label, name);
                }
                separate();
                if (avoid_level == 0 || level < avoid_level) {
                    if (strcmp(part, "EQU") == 0) {
                        p2 = match_expression(p);
                        if (p2 == 0) {
                            message(1, "bad expression");
                        } else {
                            if (assembler_step == 1) {
                                if (find_label(name)) {
                                    char m[18 + MAX_SIZE];
                                    
                                    sprintf(m, "Redefined label '%s'", name);
                                    message(1, m);
                                } else {
                                    last_label = define_label(name, instruction_value);
                                }
                            } else {
                                last_label = find_label(name);
                                if (last_label == 0) {
                                    char m[33 + MAX_SIZE];
                                    
                                    sprintf(m, "Inconsistency, label '%s' not found", name);
                                    message(1, m);
                                } else {
                                    if (last_label->value != instruction_value) {
#ifdef DEBUG
/*                                        fprintf(stderr, "Woops: label '%s' changed value from %04x to %04x\r\n", last_label->name, last_label->value, instruction_value);*/
#endif
                                        change = 1;
                                    }
                                    last_label->value = instruction_value;
                                }
                            }
                            check_end(p2);
                        }
                        break;
                    }
                    if (first_time == 1) {
#ifdef DEBUG
                        /*                        fprintf(stderr, "First time '%s' at line %d\r\n", line, line_number);*/
#endif
                        first_time = 0;
                        reset_address();
                    }
                    if (assembler_step == 1) {
                        if (find_label(name)) {
                            char m[18 + MAX_SIZE];
                            
                            sprintf(m, "Redefined label '%s'", name);
                            message(1, m);
                        } else {
                            last_label = define_label(name, address);
                        }
                    } else {
                        last_label = find_label(name);
                        if (last_label == 0) {
                            char m[33 + MAX_SIZE];
                            
                            sprintf(m, "Inconsistency, label '%s' not found", name);
                            message(1, m);
                        } else {
                            if (last_label->value != address) {
#ifdef DEBUG
/*                                fprintf(stderr, "Woops: label '%s' changed value from %04x to %04x\r\n", last_label->name, last_label->value, address);*/
#endif
                                change = 1;
                            }
                            last_label->value = address;
                        }
                        
                    }
                }
            }
            if (strcmp(part, "%IF") == 0) {
                level++;
                if (GET_UINT16(level) == 0) { if_too_deep:
                    message(1, "%IF too deep");
                    goto close_return;
                }
                if (avoid_level != 0 && level >= avoid_level)
                    break;
                undefined = 0;
                p = match_expression(p);
                if (p == 0) {
                    message(1, "Bad expression");
                } else if (undefined) {
                    message(1, "Undefined labels");
                }
                if (GET_INT16(instruction_value) != 0) {
                    ;
                } else {
                    avoid_level = level;
                }
                check_end(p);
                break;
            }
            if (strcmp(part, "%IFDEF") == 0) {
                level++;
                if (GET_UINT16(level) == 0) goto if_too_deep;
                if (avoid_level != 0 && level >= avoid_level)
                    break;
                separate();
                if (find_label(part) != 0) {
                    ;
                } else {
                    avoid_level = level;
                }
                check_end(p);
                break;
            }
            if (strcmp(part, "%IFNDEF") == 0) {
                level++;
                if (GET_UINT16(level) == 0) goto if_too_deep;
                if (avoid_level != 0 && level >= avoid_level)
                    break;
                separate();
                if (find_label(part) == 0) {
                    ;
                } else {
                    avoid_level = level;
                }
                check_end(p);
                break;
            }
            if (strcmp(part, "%ELSE") == 0) {
                if (level == 1) {
                    message(1, "%ELSE without %IF");
                    goto close_return;
                }
                if (avoid_level != 0 && level > avoid_level)
                    break;
                if (avoid_level == level) {
                    avoid_level = 0;
                } else if (avoid_level == 0) {
                    avoid_level = level;
                }
                check_end(p);
                break;
            }
            if (strcmp(part, "%ENDIF") == 0) {
                if (avoid_level == level)
                    avoid_level = 0;
                level--;
                if (level == 0) {
                    message(1, "%ENDIF without %IF");
                    goto close_return;
                }
                check_end(p);
                break;
            }
            if (avoid_level != 0 && level >= avoid_level) {
#ifdef DEBUG
                /*fprintf(stderr, "Avoiding '%s'\r\n", line);*/
#endif
                break;
            }
            if (strcmp(part, "USE16") == 0) {
                break;
            }
            if (strcmp(part, "SECTION") == 0 || strcmp(part, "SEGMENT") == 0) {
                break;  /* Output is flat: sections are ignored. */
            }
            if (strcmp(part, "CPU") == 0) {
                p = avoid_spaces(p);
                if (memcmp(p, "8086", 4) != 0)
                    message(1, "Unsupported processor requested");
                break;
            }
            if (strcmp(part, "%INCLUDE") == 0) {
                separate();
                check_end(p);
                if (part[0] != '"' || part[strlen(part) - 1] != '"') {
                    message(1, "Missing quotes on %include");
                    break;
                }
                include = 1;
                break;
            }
            if (strcmp(part, "INCBIN") == 0) {
                separate();
                check_end(p);
                if (part[0] != '"' || part[strlen(part) - 1] != '"') {
                    message(1, "Missing quotes on incbin");
                    break;
                }
                include = 2;
                break;
            }
            if (strcmp(part, "ORG") == 0) {
                p = avoid_spaces(p);
                undefined = 0;
                p2 = match_expression(p);
                if (p2 == 0) {
                    message(1, "Bad expression");
                } else if (undefined) {
                    message(1, "Cannot use undefined labels");
                } else {
                    if (first_time == 1) {
                        first_time = 0;
                        address = instruction_value;
                        start_address = instruction_value;
                        base = address;
                    } else {
                        if (instruction_value < address) {
                            message(1, "Backward address");
                        } else {
                            while (address < instruction_value)
                                emit_byte(0);
                            
                        }
                    }
                    check_end(p2);
                }
                break;
            }
            if (strcmp(part, "ALIGN") == 0) {
                p = avoid_spaces(p);
                undefined = 0;
                p2 = match_expression(p);
                if (p2 == 0) {
                    message(1, "Bad expression");
                } else if (undefined) {
                    message(1, "Cannot use undefined labels");
                } else {
                    align = address / instruction_value;
                    align = align * instruction_value;
                    align = align + instruction_value;
		    while (address < align)
		        emit_byte(0x90);
                    check_end(p2);
                }
                break;
            }
            if (first_time == 1) {
#ifdef DEBUG
                /*fprintf(stderr, "First time '%s' at line %d\r\n", line, line_number);*/
#endif
                first_time = 0;
                reset_address();
            }
            times = 1;
            if (strcmp(part, "TIMES") == 0) {
                undefined = 0;
                p2 = match_expression(p);
                if (p2 == 0) {
                    message(1, "bad expression");
                    break;
                }
                if (undefined) {
                    message(1, "non-constant expression");
                    break;
                }
                times = instruction_value;
                p = p2;
                separate();
            }
            base = address;
            g = generated;
            p3 = prev_p;
            while (times) {
                p = p3;
                separate();
                process_instruction();
                times--;
            }
            break;
        }
        if (assembler_step == 2 && listing != 0) {
            if (first_time)
                fprintf(listing, "      ");
            else
                fprintf(listing, "%04X  ", base);
            p = generated;
            while (p < (const char *)g) {
                fprintf(listing, "%02X", *p++ & 255);
            }
            while (p < (const char *)generated + sizeof(generated)) {
                fprintf(listing, "  ");
                p++;
            }
            fprintf(listing, "  %05d %s\r\n", line_number, line);
        }
        if (include == 1) {
            part[strlen(part) - 1] = '\0';
            ofs = ftell(input);
            fclose(input);
            memcpy(ibp, &level, sizeof(level)); ibp += sizeof(level);  /* We use memcpy to avoid alignment issues. TODO(pts): Do direct copy if the CPU allows it. */
            memcpy(ibp, &avoid_level, sizeof(avoid_level)); ibp += sizeof(avoid_level);
            memcpy(ibp, &line_number, sizeof(line_number)); ibp += sizeof(line_number);
            memcpy(ibp, &ofs, sizeof(ofs)); ibp += sizeof(ofs);
            fname = part + 1;
            goto do_open;
        }
        if (include == 2) {
            part[strlen(part) - 1] = '\0';
            incbin(part + 1);
        }
    }
    if (level != 1) {
        message(1, "pending %IF at end of file");
    }
  close_return:
    fclose(input);
    for (--ibp; ibp[-1] != '\0'; --ibp) {}
    --ibp;
  do_return:
    if (ibp != include_buf) {  /* Continue in file which has done the %INCLUDE of the file just finished. */
        ibp -= sizeof(ofs); memcpy(&ofs, ibp, sizeof(ofs));
        ibp -= sizeof(line_number); memcpy(&line_number, ibp, sizeof(line_number));
        ibp -= sizeof(avoid_level); memcpy(&avoid_level, ibp, sizeof(avoid_level));
        ibp -= sizeof(level); memcpy(&level, ibp, sizeof(level));
        for (fname = ibp - 1; fname[-1] != '\0'; --fname) {}
        if ((input = fopen(fname, "r")) == 0) {
            ibp = (char*)fname - 1;
            goto open_error;
        }
        if (fseek(input, ofs, SEEK_SET) != 0) {
            ibp = (char*)fname - 1;
            fprintf(stderr, "Error: cannot seek in '%s'\r\n", fname);
            goto io_error;
        }
        goto do_assemble;
    }
}

/*
 ** Main program
 */
int main(int argc, char **argv) {
    int c;
    int d;
    const char *p;
    const char *ifname;

    
    instr_open();

    /*
     ** If ran without arguments then show usage
     */
    if (argc == 1) {
        fprintf(stderr, "Typical usage:\r\n");
        fprintf(stderr, "tinasm -f bin|cmd input.asm -o output\r\n");
        exit(1);
    }
    
    /*
     ** Start to collect arguments
     */
    ifname = 0;
    output_filename = 0;
    listing_filename = 0;
    default_start_address = 0;
    c = 1;
    while (c < argc) {
        if (argv[c][0] == '-') {    /* All arguments start with dash */
            d = tolower(argv[c][1]);
            if (d == 'f') { /* Format */
                c++;
                if (c >= argc) {
                    fprintf(stderr, "Error: no argument for -f\r\n");
                    exit(1);
                } else {
                    to_lowercase(argv[c]);
                    if (strcmp(argv[c], "bin") == 0) {
                        default_start_address = 0;
                    } else if (strcmp(argv[c], "cmd") == 0) {
                        default_start_address = 0x0100;
                        cmd_format = 1;
                    } else {
                        fprintf(stderr, "Error: only 'bin', 'cmd' supported for -f (it is '%s')\r\n", argv[c]);
                        exit(1);
                    }
                    c++;
                }
            } else if (d == 'o') {  /* Object file name */
                c++;
                if (c >= argc) {
                    fprintf(stderr, "Error: no argument for -o\r\n");
                    exit(1);
                } else if (output_filename != 0) {
                    fprintf(stderr, "Error: already a -o argument is present\r\n");
                    exit(1);
                } else {
                    output_filename = argv[c];
                    c++;
                }
            } else if (d == 'l') {  /* Listing file name */
                c++;
                if (c >= argc) {
                    fprintf(stderr, "Error: no argument for -l\r\n");
                    exit(1);
                } else if (listing_filename != 0) {
                    fprintf(stderr, "Error: already a -l argument is present\r\n");
                    exit(1);
                } else {
                    listing_filename = argv[c];
                    c++;
                }
            } else if (d == 'd') {  /* Define label */
                p = argv[c] + 2;
                while (*p && *p != '=') {
                    *(char*)p = toupper(*p);
                    p++;
                }
                if (*p == '=') {
                    *(char*)p++ = 0;
                    undefined = 0;
                    p = match_expression(p);
                    if (p == 0) {
                        fprintf(stderr, "Error: wrong label definition\r\n");
                        exit(1);
                    } else if (undefined) {
                        fprintf(stderr, "Error: non-constant label definition\r\n");
                        exit(1);
                    } else {
                        define_label(argv[c] + 2, instruction_value);
                    }
                }
                c++;
            } else {
                fprintf(stderr, "Error: unknown argument %s\r\n", argv[c]);
                exit(1);
            }
        } else {
            if (ifname != 0) {
                fprintf(stderr, "Error: more than one input file name: %s\r\n", argv[c]);
                exit(1);
            } else {
                ifname = argv[c];
            }
            c++;
        }
    }
    
    if (ifname == 0) {
        fprintf(stderr, "No input filename provided\r\n");
        exit(1);
    }
    
    /*
     ** Do first step of assembly
     */
    assembler_step = 1;
    first_time = 1;
    do_assembly(ifname);
    if (!errors) {
        code_size = (long)address - start_address;
        if (cmd_format && start_address != 0x0100) {
            fprintf(stderr, "Error: -f cmd needs the code to start at 0x100 (org 0x100)\r\n");
            exit(1);
        }
        
        /*
         ** Do second step of assembly and generate final output
         */
        if (output_filename == 0) {
            fprintf(stderr, "No output filename provided\r\n");
            exit(1);
        }
        change_number = 0;
        do {
            change = 0;
            if (listing_filename != 0) {
                listing = fopen(listing_filename, "w");
                if (listing == 0) {
                    fprintf(stderr, "Error: couldn't open '%s' as listing file\r\n", output_filename);
                    exit(1);
                }
            }
            output = fopen(output_filename, "w");
            if (output == 0) {
                fprintf(stderr, "Error: couldn't open '%s' as output file\r\n", output_filename);
                exit(1);
            }
            assembler_step = 2;
            first_time = 1;
            address = start_address;
            if (cmd_format)
                cmd_header();
            do_assembly(ifname);
            if (cmd_format)
                cmd_trailer();
            if (cmd_format && (long)address - start_address != code_size) {
                code_size = (long)address - start_address;
                change = 1;
            }
            
            if (listing != 0 && change == 0) {
                fprintf(listing, "\r\n%05u ERRORS FOUND\r\n", errors);
                fprintf(listing, "%05u WARNINGS FOUND\r\n\r\n", warnings);
                fprintf(listing, "%05u PROGRAM BYTES\r\n\r\n", GET_UINT16(bytes));
                if (label_list != 0) {
                    fprintf(listing, "%-20s VALUE/ADDRESS\r\n\r\n", "LABEL");
                    sort_labels(label_list);
                }
            }
            fclose(output);
            if (listing_filename != 0)
                fclose(listing);
            if (change) {
                change_number++;
                if (change_number == 5) {
                    fprintf(stderr, "Aborted: Couldn't stabilize moving label\r\n");
                    count_error();
                }
            }
            if (errors) {
                remove(output_filename);
                if (listing_filename != 0)
                    remove(listing_filename);
                exit(1);
            }
        } while (change) ;

        exit(0);
    }

    exit(1);
}
