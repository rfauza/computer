# filename: pong_bar.ass
# isa: 3bit_v1
# 3-bit v1 ISA:
#   000  HALT     - Stop execution
#   001  MOVL     - Move literal: A -> [B:C]
#   010  ADD      - Add:      [A] + [B] -> [C]   (page 0 addresses)
#   011  SUB      - Subtract: [A] - [B] -> [C]   (page 0 addresses)
#   100  CMP      - Compare:  flags <- [0:A] vs [B:C]
#   101  JEQ      - Jump if equal:   PC <- A:B:C
#   110  JGT      - Jump if greater: PC <- A:B:C
#   111  MOVOUT   - Move out: [rampage:A] -> [B:C]
# generated_from: pong_bar.ass

001 001 000 001 # 0 - movl 1 0 1
111 001 101 000 # 1 - movout 1 5 0
111 001 101 001 # 2 - movout 1 5 1
111 001 101 010 # 3 - movout 1 5 2
100 000 000 000 # 4 - cmp 0 0 0
101 000 000 001 # 5 - JEQ start (->1)
