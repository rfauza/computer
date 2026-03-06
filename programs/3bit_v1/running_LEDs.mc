# running_LEDs.ass
# filename: running_LEDs.ass
# isa: 3bit_v1
# running_LEDs 3-bit v1
# light up one LED of the 8x8 display at a time, going down the line
# 3-bit v1 ISA:
# 000 HALT   - Stop execution
# 001 MOVL   - Move literal: litA -> [BC]
# 010 ADD    - Add: [A] + [B] -> [C]
# 011 SUB    - Subtract: [A] - [B] -> [C]
# 100 CMP    - Compare: sets flags based on [A] vs [B]
# 101 JEQ    - Jump if equal: if flag then goto ABC
# 110 JGT    - Jump if greater: if flag then goto ABC
# 111 NOP    - No operation
# setup
# row 0
# row 1
# row 2
# 3-bit v1 ISA:
#   000  HALT     - Stop execution
#   001  MOVL     - Move literal: A -> [B:C]
#   010  ADD      - Add:      [A] + [B] -> [C]   (page 0 addresses)
#   011  SUB      - Subtract: [A] - [B] -> [C]   (page 0 addresses)
#   100  CMP      - Compare:  flags <- [0:A] vs [B:C]
#   101  JEQ      - Jump if equal:   PC <- A:B:C
#   110  JGT      - Jump if greater: PC <- A:B:C
#   111  MOVOUT   - Move out: [rampage:A] -> [B:C]
# generated_from: running_LEDs.ass

001 001 000 001 # 0 - movl 1 0 1
001 010 000 010 # 1 - movl 2 0 2
001 100 000 100 # 2 - movl 4 0 4
111 100 101 000 # 3 - movout 4 5 0
111 010 101 000 # 4 - movout 2 5 0
111 001 101 000 # 5 - movout 1 5 0
111 000 101 000 # 6 - movout 0 5 0
111 100 110 000 # 7 - movout 4 6 0
111 010 110 000 # 8 - movout 2 6 0
111 001 110 000 # 9 - movout 1 6 0
111 000 110 000 # 10 - movout 0 6 0
111 100 111 000 # 11 - movout 4 7 0
111 010 111 000 # 12 - movout 2 7 0
111 001 111 000 # 13 - movout 1 7 0
111 000 111 000 # 14 - movout 0 7 0
111 100 101 001 # 15 - movout 4 5 1
111 010 101 001 # 16 - movout 2 5 1
111 001 101 001 # 17 - movout 1 5 1
111 000 101 001 # 18 - movout 0 5 1
111 100 110 001 # 19 - movout 4 6 1
111 010 110 001 # 20 - movout 2 6 1
111 001 110 001 # 21 - movout 1 6 1
111 000 110 001 # 22 - movout 0 6 1
111 100 111 001 # 23 - movout 4 7 1
111 010 111 001 # 24 - movout 2 7 1
111 001 111 001 # 25 - movout 1 7 1
111 000 111 001 # 26 - movout 0 7 1
111 100 101 010 # 27 - movout 4 5 2
111 010 101 010 # 28 - movout 2 5 2
111 001 101 010 # 29 - movout 1 5 2
111 000 101 010 # 30 - movout 0 5 2
111 100 110 010 # 31 - movout 4 6 2
111 010 110 010 # 32 - movout 2 6 2
111 001 110 010 # 33 - movout 1 6 2
111 000 110 010 # 34 - movout 0 6 2
111 100 111 010 # 35 - movout 4 7 2
111 010 111 010 # 36 - movout 2 7 2
111 001 111 010 # 37 - movout 1 7 2
111 000 111 010 # 38 - movout 0 7 2
