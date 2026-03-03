# Computes RAM[0] * RAM[1] by adding RAM[0] to accumulator RAM[2]
# filename: long_mult.ass
# isa: 3bit_v1
# Multiply by repeated addition
#   RAM[1] times. Verifies loop labels, ADD, SUB, CMP, and JGT.
#   Expected: multiplicand=3, multiplier=2 -> result=6 in RAM[2]
# ── Initialization ────────────────────────────────────────────────────────────
# ── Multiply loop ─────────────────────────────────────────────────────────────
# 3-bit v1 ISA:
#   000  HALT     - Stop execution
#   001  MOVL     - Move literal: A -> [B:C]
#   010  ADD      - Add:      [A] + [B] -> [C]   (page 0 addresses)
#   011  SUB      - Subtract: [A] - [B] -> [C]   (page 0 addresses)
#   100  CMP      - Compare:  flags <- [0:A] vs [B:C]
#   101  JEQ      - Jump if equal:   PC <- A:B:C
#   110  JGT      - Jump if greater: PC <- A:B:C
#   111  MOVOUT   - Move out: [rampage:A] -> [B:C]
# generated_from: long_mult.ass

001 011 000 000 # 0 - movl 3 0 0
001 010 000 001 # 1 - movl 2 0 1
001 000 000 010 # 2 - movl 0 0 2
001 001 000 011 # 3 - movl 1 0 3
001 000 000 100 # 4 - movl 0 0 4
010 010 000 010 # 5 - add 2 0 2
011 001 011 001 # 6 - sub 1 3 1
100 001 000 100 # 7 - cmp 1 0 4
110 000 000 101 # 8 - JGT loop (->5)
000 000 000 000 # 9 - halt
