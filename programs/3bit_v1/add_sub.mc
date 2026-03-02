# add_sub.ass
# filename: add_sub.ass
# isa: 3bit_v1
# Add/Sub 3-bit v1 test program
# tests all integer adds and subs using loops
# 3-bit v1 ISA:
#   000  HALT     - Stop execution
#   001  MOVL     - Move literal: A -> [B:C]
#   010  ADD      - Add:      [A] + [B] -> [C]   (page 0 addresses)
#   011  SUB      - Subtract: [A] - [B] -> [C]   (page 0 addresses)
#   100  CMP      - Compare:  flags <- [0:A] vs [B:C]
#   101  JEQ      - Jump if equal:   PC <- A:B:C
#   110  JGT      - Jump if greater: PC <- A:B:C
#   111  MOVOUT   - Move out: [rampage:A] -> [B:C]
# generated_from: add_sub.ass

001 000 000 000 # 0 - movl 0 0 0
001 001 000 001 # 1 - movl 1 0 1
001 001 000 010 # 2 - movl 1 0 2
001 000 000 011 # 3 - movl 0 0 3
010 000 001 100 # 4 - add 0 1 4
111 100 001 100 # 5 - movout 4 1 4
011 000 001 101 # 6 - sub 0 1 5
111 101 001 101 # 7 - movout 5 1 5
010 001 010 001 # 8 - add 1 2 1
100 001 000 011 # 9 - cmp 1 0 3
110 000 000 100 # 10 - JGT add_loop (->4)
010 000 010 000 # 11 - ADD 0 2 0
100 000 000 011 # 12 - CMP 0 0 3
110 000 000 100 # 13 - JGT add_loop (->4)
000 000 000 000 # 14 - halt
