static
double  tab[] =
{
    // 8c -S should display
	//DATA	tab<>+0(SB)/8,$(1.00000000000000000e+76)
	//DATA	tab<>+8(SB)/8,$(1.00000000000000000e+77)
	//DATA	tab<>+16(SB)/8,$(1.00000000000000000e+78)

    // but instead used to display
	//DATA	tab<>+0(SB)/8,$(1.00000000000000000e+76)
	//DATA	tab<>+8(SB)/8,$(9.99999999999999990e+76)
	//DATA	tab<>+16(SB)/8,$(1.00000000000000000e+78)
	//GLOBL	tab<>+0(SB),$24

    1.0e76, 1.0e77, 1.0e78
};
