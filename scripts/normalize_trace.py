"""
normalize_trace.py — Convert a raw RISC-V execution trace into a normalized, 
separated format suitable for ingestion by the performance model.

Input: a trace file where each line contains (at minimum) a PC and hex-encoded
instruction encoding in the format produced by spike or a compatible simulator.

For example:
core   0: 3 0x0000000000001008 (0xf1402573) x10 0x0000000000000000
core   0: 3 0x000000000000100c (0x0182b283) x5  0x0000000080000000 mem 0x0000000000001018

Output: one line per user-space instruction with the fields:
    PC  TYPE  OPCODE  DST  SRC1  SRC2  TAKEN  MEM_ADDR

  - PC       : program counter (hex, as-is from the trace)
  - TYPE     : instruction class — LOAD | STORE | BRANCH | JAL | JALR | ALU
  - OPCODE   : full instruction encoding (32-bit or 16-bit compressed), hex
  - DST      : destination register index, or '-' if none
  - SRC1/2   : source register indices, or '-' if unused
  - TAKEN    : 'Y' | 'N' for branches, '-' otherwise
  - MEM_ADDR : always '-' (in process ...)

Kernel-space instructions (PC >= 0xffffffc000000000) are  dropped.
Both 32-bit standard and 16-bit compressed (RVC) encodings are supported.

Usage:
    python normalize_trace.py <trace_file>
"""
import sys

INSTRUCTION_OPCODES = {
    0b0000011: "LOAD",
    0b0100011: "STORE",
    0b1100011: "BRANCH",
    0b1101111: "JAL",
    0b1100111: "JALR"
}


def categorize_reduced(encoding):
    """Returns the type of a reduced instruction"""
    quadrant = encoding & 0b11
    funct3 = (encoding >> 13) & 0b111

    if quadrant == 0:
        if funct3 == 0b010 or funct3 == 0b011:
            return "LOAD"
        elif funct3 == 0b110 or funct3 == 0b111:
            return "STORE"
        else:
            return "ALU"
    elif quadrant == 1:
        if funct3 == 0b101:
            return "JAL"
        elif funct3 == 0b110 or funct3 == 0b111:
            return "BRANCH"
        else:
            return "ALU"
    else:
        if funct3 == 0b010 or funct3 == 0b011:
            return "LOAD"
        elif funct3 == 0b110 or funct3 == 0b111:
            return "STORE"
        elif funct3 == 0b100:
            rs2 = (encoding >> 2) & 0b11111
            if (rs2 == 0):
                return "JALR"
            return "ALU"
        else:
            return "ALU"


def categorize_by_opcode(encoding, opcode):
    """Returns the type of an instruction (LOAD, STORE, BRANCH, JAL, JALR, ALU)."""
    if (encoding & 0b11) != 3:
        return categorize_reduced(encoding)

    if opcode in INSTRUCTION_OPCODES:
        return INSTRUCTION_OPCODES[opcode]
    return "ALU"


def fields_used(opcode):
    """Returns (uses_rs1, uses_rs2, writes_rd) for a 32-bit opcode."""
    if opcode == 0x37 or opcode == 0x17:   # lui, auipc
        return (False, False, True)
    if opcode == 0x6F:                     # jal
        return (False, False, True)
    if opcode == 0x67:                     # jalr
        return (True, False, True)
    if opcode == 0x63:                     # branches
        return (True, True, False)
    if opcode == 0x03:                     # loads
        return (True, False, True)
    if opcode == 0x23:                     # stores
        return (True, True, False)
    if opcode == 0x13 or opcode == 0x1B:   # I-type ALU (addi, slli, etc.)
        return (True, False, True)
    if opcode == 0x33 or opcode == 0x3B:   # R-type ALU (add, sub, mul, etc.)
        return (True, True, True)
    if opcode == 0x73:                     # system (csr*, ecall, ebreak)
        return (True, False, True)         # close enough — csr reads rs1
    return (False, False, False)            # unknown


def decode_sources_compressed(encoding):
    """Returns (rs1, rs2, dst) for a 16-bit compressed instruction."""
    quadrant = encoding & 0b11
    funct3 = (encoding >> 13) & 0b111

    # Helper: 3-bit "compressed register" x8-x15
    def cr3(shift):
        return ((encoding >> shift) & 0b111) + 8

    if quadrant == 0:
        if funct3 == 0b010 or funct3 == 0b011:  # c.lw, c.ld
            return (cr3(7), "-", cr3(2))
        if funct3 == 0b110 or funct3 == 0b111:  # c.sw, c.sd
            return (cr3(7), cr3(2), "-")
        if funct3 == 0b000:                     # c.addi4spn (rd' = sp + imm)
            return (2, "-", cr3(2))
        return ("-", "-", "-")

    if quadrant == 1:
        rd_full = (encoding >> 7) & 0b11111
        if funct3 == 0b000:                     # c.addi / c.nop
            return (rd_full, "-", rd_full)
        if funct3 == 0b001:                     # c.addiw (RV64)
            return (rd_full, "-", rd_full)
        if funct3 == 0b010:                     # c.li
            return ("-", "-", rd_full)
        if funct3 == 0b011:                     # c.lui or c.addi16sp
            if rd_full == 2:                    # c.addi16sp (rd = sp)
                return (2, "-", 2)
            return ("-", "-", rd_full)          # c.lui
        if funct3 == 0b100:                     # ALU ops: c.sub, c.xor, c.or, c.and, c.srli, c.srai, c.andi
            return (cr3(7), cr3(2), cr3(7))
        if funct3 == 0b101:                     # c.j — no registers
            return ("-", "-", "-")
        if funct3 == 0b110 or funct3 == 0b111:  # c.beqz, c.bnez
            return (cr3(7), 0, "-")
        return ("-", "-", "-")

    rd_full = (encoding >> 7) & 0b11111
    rs2_full = (encoding >> 2) & 0b11111
    if funct3 == 0b000:                         # c.slli
        return (rd_full, "-", rd_full)
    if funct3 == 0b010 or funct3 == 0b011:      # c.lwsp, c.ldsp
        return (2, "-", rd_full)
    if funct3 == 0b100:                         # mixed: c.jr, c.jalr, c.mv, c.add
        bit12 = (encoding >> 12) & 1
        if rs2_full == 0:
            if bit12 == 0:                      # c.jr
                return (rd_full, "-", "-")
            else:                               # c.jalr (rd implicitly x1)
                return (rd_full, "-", 1)
        else:
            if bit12 == 0:                      # c.mv
                return ("-", rs2_full, rd_full)
            else:                               # c.add
                return (rd_full, rs2_full, rd_full)
    if funct3 == 0b110 or funct3 == 0b111:      # c.swsp, c.sdsp
        return (2, rs2_full, "-")
    return ("-", "-", "-")


def decode_sources(encoding, opcode):
    """Returns the destination and sources of an instruction."""
    if (encoding & 0b11) != 3:
        return decode_sources_compressed(encoding)

    used_fields = fields_used(opcode)
    rs1, rs2, dst = "-", "-", "-"

    if (used_fields[0]):
        rs1 = (encoding >> 15) & 0b11111
    if (used_fields[1]):
        rs2 = (encoding >> 20) & 0b11111
    if (used_fields[2]):
        dst = (encoding >> 7) & 0b11111
    return [rs1, rs2, dst]


def normalize(path_r, path_w):
    """
    Normalizes the compact log into
    PC, TYPE, OPCODE, DST, SRC1, SRC2, TAKEN, MEM_ADDR(-)
    """

    # Read the file and split into lines
    with open(path_r, 'r') as in_file, open(path_w, 'w') as out_file:
        prev = None
        # Parse and decode each line into its corresponding fields
        for line in in_file:
            fields = line.split()
            if len(fields) < 5:
                continue

            encoding = int(fields[4].replace("(", "").replace(")", ""), 16)
            opcode = encoding & 0b1111111

            pc = fields[3]

            # Filter kernel instructions
            pc_int = int(pc, 16)
            if pc_int >= 0xffffffc000000000:
                continue

            instr_type = categorize_by_opcode(encoding, opcode)
            rs1, rs2, dst = decode_sources(encoding, opcode)
            taken_memory = "-"

            # Determines whether branch was taken provided that we have previous instruction
            width = 2 if (encoding & 0b11) != 3 else 4
            if prev is not None:
                taken = compute_taken(prev, pc_int)
                emit(out_file, prev, taken)

            prev = (pc, instr_type, encoding, dst, rs1, rs2, width, pc_int)

        # Flush the last instruction
        if prev is not None:
            emit(out_file, prev, taken="-")


def compute_taken(prev, next_pc):
    """Returns whether the branch was taken or not. Chose N and Y instead of NOT_TAKEN and TAKEN, respectively, to optimize the speed of paring in C++."""
    pc, instr_type, encoding, dst, rs1, rs2, width, pc_int = prev
    if instr_type != "BRANCH":
        return "-"
    jump = pc_int + width
    if jump != next_pc:
        return "Y"          # Branch is taken
    return "N"              # Branch is not taken


def emit(out_file, prev, taken):
    """Emits a normalized instruction into the output file"""
    pc, instr_type, encoding, dst, rs1, rs2, width, pc_int = prev
    out_file.write(f"{pc} {instr_type} 0x{encoding:08x} {dst} {rs1} {rs2} {taken} -\n")


def main():
    if len(sys.argv) <= 2:
        raise ValueError(f"Usage: {sys.argv[0]} <trace_file> <output_file>")
    path_r = sys.argv[1]
    path_w = sys.argv[2]
    normalize(path_r, path_w)


if __name__ == "__main__":
    main()
