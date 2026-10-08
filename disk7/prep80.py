#!/usr/bin/env python3
"""Turn the DR MAC source of disk77b into 8080 source that XLT86 translates
into something ASM86 accepts.

- expand the Z80-extension macros (JR, JRC, JRNC, JRZ, JRNZ, DJNZ) into the
  8080 code they generate when Z80 is FALSE, and drop their definitions:
  XLT86 has no macro processor
- rewrite ELSE as ENDIF + IF NOT (condition): DR ASM and XLT86 have no ELSE
- move a label off ORG and END, and drop END's start address
- define RING after LAST instead of forward-referencing it
- rename symbols that are ASM86 reserved words or 8086 mnemonics
- split long DB strings: XLT86 cuts long lines, losing the closing quote

usage: prep80.py disk77b.mac > disk77b.asm
"""
import re
import sys

JUMPS = {"JR": "JMP", "JRC": "JC", "JRNC": "JNC", "JRZ": "JZ", "JRNZ": "JNZ"}
RENAME = {"ESC": "ESCCHR", "LAST": "LASTADR", "LIST": "LISTDEV",
          "LOOP": "DLOOP", "TYPE": "TYPECHR"}
MAXSTR = 32

IDENT = re.compile(r"[A-Za-z@?$_][A-Za-z0-9@?$_]*")


def split_code(line):
    """Split a line into code and comment, ignoring ';' inside quotes."""
    quoted = False
    for i, c in enumerate(line):
        if c == "'":
            quoted = not quoted
        elif c == ";" and not quoted:
            return line[:i], line[i:]
    return line, ""


def rename(code):
    out, i, quoted = [], 0, False
    while i < len(code):
        c = code[i]
        if c == "'":
            quoted = not quoted
        if not quoted:
            m = IDENT.match(code, i)
            if m and (i == 0 or not re.match(r"[A-Za-z0-9@?$_]", code[i - 1])):
                word = m.group(0)
                out.append(RENAME.get(word.upper(), word))
                i = m.end()
                continue
        out.append(c)
        i += 1
    return "".join(out)


def db_items(operands):
    """Split DB operands on commas outside quotes."""
    items, cur, quoted = [], "", False
    for c in operands:
        if c == "'":
            quoted = not quoted
        if c == "," and not quoted:
            items.append(cur.strip())
            cur = ""
        else:
            cur += c
    items.append(cur.strip())
    return items


def chunks(item):
    """Cut a quoted string into pieces, never inside a doubled quote."""
    body, pieces, cur, i = item[1:-1], [], "", 0
    while i < len(body):
        step = 2 if body[i:i + 2] == "''" else 1
        if len(cur) + step > MAXSTR:
            pieces.append("'" + cur + "'")
            cur = ""
        cur += body[i:i + step]
        i += step
    pieces.append("'" + cur + "'")
    return pieces


def split_db(label, operands, comment):
    items = db_items(operands)
    if not any(i.startswith("'") and len(i) - 2 > MAXSTR for i in items):
        return None
    lines, group = [], []
    for item in items:
        if item.startswith("'") and len(item) - 2 > MAXSTR:
            if group:
                lines.append(group)
                group = []
            for piece in chunks(item):
                lines.append([piece])
        else:
            group.append(item)
    if group:
        lines.append(group)
    out = []
    for n, group in enumerate(lines):
        out.append("%s\tDB\t%s%s" % (label if n == 0 else "", ",".join(group),
                                     "\t" + comment if n == 0 and comment else ""))
    return out


def main():
    text = open(sys.argv[1], encoding="latin-1").read()
    text = text.replace("\r", "").split("\x1a")[0]
    out = []
    skip = lastmac = False
    cond = ""
    ring = None
    for line in text.split("\n"):
        if line.startswith("$-MACRO"):
            continue
        if re.match(r"@GENDD\s+MACRO", line):
            skip = True
        if skip:
            if re.match(r"DJNZ\s+MACRO", line):
                lastmac = True
            if lastmac and line.split()[:1] == ["ENDM"]:
                skip = False
            continue

        code, comment = split_code(line)
        code = rename(code)
        m = re.match(r"^(\S*)\s*(\S*)\s*(.*?)\s*$", code)
        label, op, operands = m.group(1), m.group(2).upper(), m.group(3)

        if op == "IF":
            cond = operands
        if op == "ELSE":
            out += ["\t ENDIF", "\t IF\tNOT (%s)" % cond]
            continue
        if op == "SET" and label.upper() == "RING":
            ring = "%s\tEQU\t%s" % (label, operands)
            continue
        if op == "END":
            if label:
                out.append("%s\tEQU\t$" % label)
            if ring:
                out.append(ring)
            out.append("\tEND")
            continue
        if op == "ORG" and label:
            out += ["\tORG\t%s" % operands, "%s\tEQU\t$" % label]
            continue
        if op in JUMPS:
            out.append("%s\t%s\t%s%s" % (label, JUMPS[op], operands,
                                         "\t" + comment if comment else ""))
            continue
        if op == "DJNZ":
            out += ["%s\tDCR\tB" % label,
                    "\tJNZ\t%s%s" % (operands, "\t" + comment if comment else "")]
            continue
        if op == "DB":
            split = split_db(label, operands, comment)
            if split:
                out += split
                continue
        out.append(code + comment)
    sys.stdout.write("\r\n".join(out) + "\r\n")


main()
