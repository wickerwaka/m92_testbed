TEST( "NOP", just_nop, 27 )
TEST( "DEC AX", dec_ax, 19 )
TEST( "DEC AL", dec_al, 19 )

TEST( "MOV AX, DX", mov_reg_reg, 19 )
TEST( "MOV AX, [DI]", mov_reg_mem, 35 )
TEST( "MOV [DI], AX", mov_mem_reg, 43 )

TEST( "MOV AX, SS:[DI+4]", mov_from_mem_seg, 59 )
TEST( "MOV SS:[DI+4], AX", mov_to_mem_seg, 59 )
TEST( "MOV AX, [DI+4]", mov_from_mem, 49 )
TEST( "MOV [DI+4], AX", mov_to_mem, 44 )
TEST( "ADD [DI], AX", add_mem, 68 )
TEST( "ADD SS:[DI], AX", add_mem_seg, 69 )
TEST( "ROR DX, 0", ror0, 36 )
TEST( "ROR DX, 1", ror1, 19 )
TEST( "ROR DX, 2", ror2, 52 )
TEST( "ROR DX, 4", ror4, 69 )
TEST( "ROR DX, 8", ror8, 101 )
