0 load #0     ; establish fake stack frame containing new value for FP
1 load #4     ; i.e., 4
2 rsf         ; load FP using a Remove Stack Frame instruction
3 inc   5     ; adjust the frame pointer
4 load #-1    ; set the display register values to a marker "invalid"
5 store 0
6 load #-1
7 store 1
8 load #-1
9 store 2
