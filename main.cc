#include <cassert>
#include <iostream>

#include "posits.h"

/*
 * Test cases for ps = 16, es = 3
 */

void test_fTOp()
{
    assert(fTOp(1.0f) == 16384);
    assert(fTOp(0.25f) == 14336);
    assert(fTOp(128.5f) == 23556);
    assert(fTOp(15138816.0f) == 30670);
    assert(fTOp(0.00982666015625) == 9480);

    assert((fTOp(-0.25f) & mask(ps, 0)) == 51200);
    assert((fTOp(-239.5f) & mask(ps, 0)) == 41092);
}

void test_iTOp()
{
    assert(iTOp(1) == 16384);
    assert(iTOp(15184) == 27573);

    assert((iTOp(-1) & mask(ps, 0)) == 49152);
    assert((iTOp(-15184)  & mask(ps, 0)) == 37963);
}

void test_enc_dec()
{
    for(int i = 0; i <= mask(ps, 0); i++)
    {
        if((encode(decode(i)) & mask(ps, 0)) != i)
        {
            assert(i == encode(decode(i)));
        }
    }
}

void test_comp()
{
    assert(eq(65503, 65503));

    assert(lt(65513, 65507));
    assert(lt(32769, 32767));
    assert(lt(32743, 32753));

    assert(!gt(65513, 65507));
    assert(!gt(32769, 32767));
    assert(!gt(32743, 32753));

    assert(le(65513, 65507));
    assert(le(32769, 32767));
    assert(le(32743, 32753));

    assert(!ge(65513, 65507));
    assert(!ge(32769, 32767));
    assert(!ge(32743, 32753));

    assert(le(1, 1));
    assert(ge(2, 2));

    assert(!le(2, 1));
    assert(!ge(2, 3));
}

void test_addition()
{
    assert(addition(31385, 31397) == 31519);
    assert(addition(32757, 1) == 32757);
    assert(addition(32757, 65532) == 32757);
    assert((addition(61005, 60997) & mask(ps, 0)) == 60489);
    assert(addition(32767, 32769) == 0);
    assert((addition(17408, 47616) & mask(ps, 0)) == 49152);
}

void test_substraction()
{
    for(int i = 0; i <= mask(ps, 0); i++)
    {
        assert(substraction(i, i) == 0);
    }
}

void test_multiplication()
{
    /* Neutral element check */
    for(int i = 0; i <= mask(ps, 0); i++)
    {
        assert((multiplication(i, 16384) & mask(ps, 0)) == i);
    }

    assert(multiplication(11665, 12848) == 8270);
    assert((multiplication(26110, 52224) & mask(ps, 0)) == 40964);
    assert((multiplication(32767, 32767) & mask(ps, 0)) == 32767);
    assert((multiplication(1, 1) & mask(ps, 0)) == 1);
    assert((multiplication(65535, 65535) & mask(ps, 0)) == 1);
}

void test_division()
{
    /* Neutral element check */
    for(int i = 0; i <= mask(ps, 0); i++)
    {
        assert((division(i, 16384) & mask(ps, 0)) == i);
    }

    /* Check for division by zero */
    for(int i = 0; i <= mask(ps, 0); i++)
    {
        assert((division(i, 0) & mask(ps, 0)) == 32768);
    }

    /* Overflows */
    assert(division(32767, 1) == 32767);
    assert(division(1, 32767) == 1);

    assert((division(32465, 35583) & mask(ps, 0)) == 34621);
    assert(division(32583, 32604) == 14994);
    assert((division(15616, 48666) & mask(ps, 0)) == 50488);
}

void test_unaryMinus()
{
    Posit p1((int64_t)24);
    assert(((-p1).val & mask(ps, 0)) == 44544);
}

void test_toInt()
{
    Posit p1((int64_t)3);
    Posit p2((int64_t)87);
    assert((p1 + p2).toInt() == 90);
    assert((p1 - p2).toInt() == -84);
    assert((p1 * p2).toInt() == 261);
    assert((p1 / p2).toInt() == 0);
}

void test_toFloat()
{
    Posit p1((int64_t)3);
    Posit p2((int64_t)87);
    assert(((p1 + p2).toFloat()) > 89.9f && ((p1 + p2).toFloat()) < 90.1f);
    assert((p1 - p2).toFloat() < -83.9f && (p1 - p2).toFloat() > -84.1f);
    assert((p1 * p2).toFloat() > 260.9f && (p1 * p2).toFloat() < 261.1f);
    assert((p1 / p2).toFloat() > 0.0344f && (p1 / p2).toFloat() < 0.0345f);
    assert((p2 / p1).toFloat() > 28.9f && (p2 / p1).toFloat() < 29.1f);
}

int main(int argc, char **argv)
{
    test_fTOp();
    test_iTOp();
    test_enc_dec();
    test_comp();
    test_addition();
    test_substraction();
    test_multiplication();
    test_division();
    test_unaryMinus();
    test_toInt();
    test_toFloat();

    return 0;
}
