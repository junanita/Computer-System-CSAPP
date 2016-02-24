/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * Name: Ningna Wang
 * AndrewId: ningnaw
 *
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
//1
/* 
 * thirdBits - return word with every third bit (starting from the LSB) set to 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 1
 */
int thirdBits(void) {
	int i = 0x49; 		// i = 		0000 0000 0000 0000 0000 0000 0100 1001
	int j = i << 9 | i;	// j = 		0000 0000 0000 0000 1001 0010 0100 1001
	return j << 18 | j;  	// result = 	0100 1001 0010 0100 1001 0010 0100 1001
}
/*
 * isTmin - returns 1 if x is the minimum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmin(int x) {
  // return !(x ^ (1 << 31)); // if x == Tmin, x XOR Tmin will return 0, otherwise return 1
  return (!(x + x)) ^ (!x);
}
//2
/* 
 * isNotEqual - return 0 if x == y, and 1 otherwise 
 *   Examples: isNotEqual(5,5) = 0, isNotEqual(4,5) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int isNotEqual(int x, int y) {
  return !!(x ^ y);
}
/* 
 * anyOddBit - return 1 if any odd-numbered bit in word set to 1
 *   Examples anyOddBit(0x5) = 0, anyOddBit(0x7) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int anyOddBit(int x) {
    int m1 = (0xAA << 8) | 0xAA;  // 0000 0000 0000 0000 1010 1010 1010 1010
    int m2 = m1 << 16;            // 1010 1010 1010 1010 0000 0000 0000 0000
    int m3 = m1 | m2;             // 1010 1010 1010 1010 1010 1010 1010 1010

    return !!(x & m3);
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  return ~x + 1;
}
//3
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
  int notP = !x;
  notP = notP << 31 >> 31;
  //return (y & (notP-1)) | (z & (~(notP-1)));
  return ((~notP) & y) | (notP & z);
}
/* 
 * subOK - Determine if can compute x-y without overflow
 *   Example: subOK(0x80000000,0x80000000) = 1,
 *            subOK(0x80000000,0x70000000) = 0, 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int subOK(int x, int y) {
  
  // overflow when:
  // x and y has different sign
  // and
  // x and x-y has different sign
  int signX = x >> 31;
  int signY = y >> 31;
  int signRes = (x + (~y + 1)) >> 31;
  return !((signX ^ signY) & (signX ^ signRes));
}
/* 
 * isGreater - if x > y  then return 1, else return 0 
 *   Example: isGreater(4,5) = 0, isGreater(5,4) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isGreater(int x, int y) {
  int signX = x >> 31;
  int signY = y >> 31;
  int signEqual = !(signX ^ signY);
  int signNotEqual = signX & (!signY);

  // if sign equal, then x > y => sign of x + (~y) is 0
  // otherwise 1
  int isBiggerWhenEqual = signEqual & ((x + (~y)) >> 31);
  //int isBiggerNotEqual = signNotEqual & !((x + (~y)) >> 31);
  return !(isBiggerWhenEqual | signNotEqual);
}
//4
/*
 * bitParity - returns 1 if x contains an odd number of 0's
 *   Examples: bitParity(5) = 0, bitParity(7) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 4
 */
int bitParity(int x) {
  // When using XOR for 2bits, eg 1^0 = 1, so 01,10 will return 1
  int x16 = (x >> 16) ^ x;
  int x8 = (x16 >> 8) ^ x16;
  int x4 = (x8 >> 4) ^ x8;
  int x2 = (x4 >> 2) ^ x4;
  int x1 = (x2 >> 1) ^ x2;
  return (x1 & 1);
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
  int signX = x >> 31;
  int positiveX = ((~x) & signX) | ((~signX) & x);

  
  // log2(n) + 2
  int temp, bias;
  temp = (!!(positiveX >> 16)) << 4;

  temp = temp | (!!(positiveX >> (temp + 8))) << 3;
  temp = temp | (!!(positiveX >> (temp + 4))) << 2;
  temp = temp | (!!(positiveX >> (temp + 2))) << 1;
  temp = temp | (positiveX >> (temp + 1));

  bias = ~(!(positiveX ^ 0)) + 1; 
  return temp + 2 + bias;

}
//float
/* 
 * float_half - Return bit-level equivalent of expression 0.5*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_half(unsigned uf) {
  // since uf is unsigned, all int will be casted to unsigned
  int exp_f = (uf & (0xFF << 23)) >> 23;
  int sign_f = uf >> 31;
  int frac_f = uf & 0x7FFFFF;
  //int e_f = exp_f -127;
  
  // nan
  if (exp_f == 0xFF)
	return uf;
  // *0.5 basically means right shift if exp_f > 1
  if (exp_f > 1) 
	return (sign_f << 31) | ((exp_f - 1) << 23) | frac_f;

  frac_f |= (exp_f << 23);
  frac_f = (uf & 0x00FFFFFF) >> 1;
  frac_f += ((uf & 3) >> 1) & (uf & 1);
  return (sign_f << 31) | frac_f;
}
/* 
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {
  unsigned sign_x = x > 0 ? 0 : 1;
  unsigned abs_x = x > 0 ? x : -x;
  unsigned exp = 0;
  unsigned temp = abs_x;
  unsigned carry = 0;
  unsigned t = 0;

  if (!x) return 0;

  while (1) {
	t = temp;
	temp = temp << 1;
	exp = exp + 1;
	if (t & 0x80000000)
		break;
  }
  if ((temp & 0x1FF) > 0x100 || (temp & 0x3FF) ==  0x300)
	carry = 1;
  
  return (sign_x << 31) + ((159 - exp) << 23) + (temp >> 9) + carry; 

  
}
/* 
 * float_f2i - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int float_f2i(unsigned uf) {
  // since uf is unsigned, all int will be casted to unsigned 
  int exp_f = (uf & (0xFF << 23)) >> 23;
  int sign_f = uf >> 31;
  int frac_f = uf & 0x7FFFFF;
  int e_f = exp_f - 127;

  // nan and infinity
  if (exp_f == 0xFF || e_f > 30)
	return 0x80000000u;
 
 // there's a right shift || exp is 0
  if (e_f < 0 || !exp_f) 
	return 0;
  
  // add 1 to head: normalized
  frac_f |= (1 << 23);
  if (e_f < 23) 
	frac_f = frac_f >> (23 - e_f);
  else 
	frac_f = frac_f << (e_f - 23);

 // negate if negative
 if (sign_f & 1)
	return ~frac_f + 1;

  return frac_f;
}
