#pragma once

#include <bit>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <algorithm>

#include <cstdint>

typedef std::uint64_t posit_t;

typedef struct unum
{
    bool sign;
    int64_t exponent;
    int64_t fraction;
} unum_t;

static int64_t ps = 16;
static int64_t es = 3;

/* Create mask with n ones and k trailing zeros */
int64_t mask(int64_t n, int64_t k)
{
    if(n) return (((1 << (n)) - 1) << (k));
    else return 0;
}

/* The number of bits needed to represent a number */
int64_t bits(int64_t n)
{
    uint64_t t;
    if(n < 0) t = (-n);
    else t = n;
    return std::bit_width(t);
}

/* Remove trailing zeros */
int64_t trail(int64_t n)
{
    return (n >> std::countr_zero((uint64_t)n));
}

int64_t norm(int64_t frac)
{
    if(bits(frac) > ps)
    {
        frac >>= bits(frac) - ps;
    }
    frac = trail(frac);
    return frac;
}

/* Add posits */
unum_t add(unum_t a, unum_t b)
{
    unum_t r = {.sign = 0, .exponent = 0, .fraction = 0};

    /* Align fractions */
    a.fraction = trail(a.fraction);
    b.fraction = trail(b.fraction);

    int64_t bits_a = bits(a.fraction);
    int64_t bits_b = bits(b.fraction);

    if(bits_a > bits_b) b.fraction <<= std::abs(bits_a - bits_b);
    if(bits_a < bits_b) a.fraction <<= std::abs(bits_a - bits_b);

    /* Get hidden bit position */
    int64_t pos = bits(a.fraction);

    /* Shift fractions according to exponent */
    if(a.exponent > b.exponent) b.fraction >>= std::abs(a.exponent - b.exponent);
    if(a.exponent < b.exponent) a.fraction >>= std::abs(a.exponent - b.exponent);

    /* Init result exponent */
    r.exponent = std::max(a.exponent, b.exponent);

    /* Add fractions*/
    r.fraction = ((a.sign) ? -1 : 1) * a.fraction + ((b.sign) ? -1 : 1) * b.fraction;
    r.sign = (r.fraction < 0);
    r.fraction = std::abs(r.fraction);

    /* Normalize */
    int64_t result_pos = bits(r.fraction);
    r.exponent += (result_pos - pos);
    r.fraction >>= std::max((int64_t)0, (bits(r.fraction) - ps)); /* Shrink fraction to prevent overflow */
    r.fraction = trail(r.fraction);

    /* Check for zero */
    if(r.fraction == 0)
    {
        r.sign = 0;
        r.exponent = 0;
    }

    /* Normalize Fraction */
    r.fraction = norm(r.fraction);

    return r;
}

/* Sub posits */
unum_t sub(unum_t a, unum_t b)
{
    b.sign = !b.sign;
    return add(a, b);
}

unum_t mul(unum_t a, unum_t b)
{
    unum_t r = {.sign = 0, .exponent = 0, .fraction = 0};

    /* Align fractions */
    a.fraction = trail(a.fraction);
    b.fraction = trail(b.fraction);

    int64_t bits_a = bits(a.fraction);
    int64_t bits_b = bits(b.fraction);

    if(bits_a > bits_b) b.fraction <<= std::abs(bits_a - bits_b);
    if(bits_a < bits_b) a.fraction <<= std::abs(bits_a - bits_b);

    /* Multiplication */
    r.sign = a.sign ^ b.sign;
    r.exponent = a.exponent + b.exponent;
    r.fraction = a.fraction * b.fraction;

    /* Adjust exponent properly */
    r.exponent += (bits(r.fraction) - 1) - (bits(a.fraction) - 1) - (bits(b.fraction) - 1);

    /* Normalize Fraction */
    r.fraction = norm(r.fraction);

    return r;
}

unum_t div(unum_t a, unum_t b)
{
    unum_t r = {.sign = 0, .exponent = 0, .fraction = 0};

    /* Check for special cases */
    if(((a.sign == 1) && (a.fraction == 0)))
    {
        r.sign = 1;
        return r;   /* Return NaR */
    }
    if(b.fraction == 0)
    {
        r.sign = 1;
        return r;   /* Return NaR cause of division by zero */
    }
    if(a.fraction == 0)
    {
        return r;   /* Return Zero */
    }

    /* Align fractions */
    a.fraction = trail(a.fraction);
    b.fraction = trail(b.fraction);

    int64_t bits_a = bits(a.fraction);
    int64_t bits_b = bits(b.fraction);

    if(bits_a > bits_b) b.fraction <<= std::abs(bits_a - bits_b);
    if(bits_a < bits_b) a.fraction <<= std::abs(bits_a - bits_b);

    /* Division */
    r.sign = a.sign ^ b.sign;
    r.exponent = a.exponent - b.exponent;
    r.fraction = ((a.fraction << ps) / b.fraction);

    /* Handle numbers smaller 1 */
    if(a.fraction < b.fraction)
    {
        r.exponent -= 1;
    }

    /* Normalize Fraction */
    r.fraction = norm(r.fraction);

    return r;
}

/* Decode posit */
unum_t decode(posit_t p)
{
    unum_t r = {.sign = 0, .exponent = 0, .fraction = 0};

    /* Check for special numbers */
    if(p == 0)
    {
        /* Zero */
        return r;
    }
    else if(p == (1 << (ps - 1)))
    {
        /* NaR */
        r.sign = 1;
        return r;
    }

    /* Check for sign */
    if((p >> ((ps - 1)) & 1))
    {
        r.sign = 1;
        p = (~p) + 1;
    }

    /* Decode regime */
    int64_t regime;
    int64_t regime_size;
    if(p >> (ps - 2) & 1)
    {
        regime = std::countl_one((uint64_t)(p << (64 - ps + 1))) - 1;
        regime_size = std::min(ps, regime + 2);
    }
    else
    {
        regime = -(std::countl_zero((uint64_t)(p << (64 - ps + 1))));
        regime_size = (-regime) + 1;
    }

    /* Exponent */
    int64_t exponent_size = std::max((int64_t)0, std::min(es, ps - 1 - regime_size));
    /* Fraction */
    int64_t fraction_size = std::max((int64_t)0, ps - 1 - regime_size - exponent_size);

    /* Extract values */
    int64_t exponent = (((p >> fraction_size) & mask(exponent_size, 0)) << (es - exponent_size));
    r.exponent = regime * (1 << es) + exponent;
    r.fraction = ((p & mask(fraction_size, 0)) | (1 << fraction_size)); /* hidden bit */

    /* Normalize */
    int64_t trz = std::countr_zero((uint64_t)r.fraction);
    r.fraction >>= trz;

    return r;
}

/* Encode posit */
posit_t encode(unum_t p)
{
    posit_t r = 0;

    /* Check for special values */
    if(p.sign == 0 && p.fraction == 0)
    {
        /* Zero */
        return 0;
    }
    else if(p.sign == 1 && p.fraction == 0)
    {
        /* NaR */
        return (1 << (ps -1));
    }

    int64_t regime = (p.exponent >> es);
    int64_t exponent = (p.exponent & mask(es, 0));

    /* Check overflow */
    if(regime >= (ps - 2))
    {
        if(!p.sign)
        {
            return (1 << (ps - 1)) - 1;
        }
        else
        {
            return (1 << (ps - 1)) + 1;
        }
    }
    else if(regime < -(ps - 2))
    {
        return 1;
    }

    /* Regime */
    int64_t regime_size;
    if(regime >= 0)
    {
        regime_size = std::min(ps - 1, regime + 2);
        r |= mask(regime_size - 1, ps - regime_size);
    }
    else
    {
        regime_size = (-regime) + 1;
        r |= (1 << (ps - 1 - regime_size));
    }

    /* Exponent */
    int64_t exponent_size = std::min(es, ps - 1 - regime_size);
    /* Fraction */
    int64_t fraction_size = ps - 1 - regime_size - exponent_size;

    /* Remove trailing zeros of fraction */
    p.fraction = trail(p.fraction);

    /* Rounding related variables */
    int64_t exp_frac_before_trunc = (exponent << (bits(p.fraction))) | p.fraction;
    int64_t exp_frac_before_trunc_length = es + bits(p.fraction);

    /* Remove hidden bit of fraction */
    int64_t fraction_length = std::bit_width((uint64_t)p.fraction) - 1;
    p.fraction &= mask(fraction_length, 0);

    /* Add exponent */
    r |= ((exponent >> (es - exponent_size)) & mask(exponent_size, 0)) << fraction_size;
    /* Add fraction */
    r |= ((p.fraction >> std::max((int64_t)0, (fraction_length - fraction_size))) & mask(fraction_size, 0)) << std::max((int64_t)0, (fraction_size - fraction_length));

    /* Rounding */
    int64_t expfrac = (exponent << fraction_length) | (p.fraction & mask(fraction_length, 0));
    if(exp_frac_before_trunc_length > exponent_size + fraction_length)
    {
        int64_t overflow_size = exp_frac_before_trunc_length - (exponent_size + fraction_length);
        int64_t overflow = expfrac & mask(overflow_size, 0);
        /* Check for round up */
        if(overflow > (1 << (overflow_size - 1)))
        {
            r = r + 1;
        }
    }

    /* Sign */
    if(p.sign == 1)
    {
        r = (~r) + 1;
    }

    return r;
}

bool eq(posit_t a, posit_t b)
{
    return (a == b);
}

bool lt(posit_t a, posit_t b)
{
    if((a >> (ps - 1)) & 1) a = (~a) + 1;
    if((b >> (ps - 1)) & 1) b = (~b) + 1;
    return ((int64_t)a < (int64_t)b);
}

bool gt(posit_t a, posit_t b)
{
    if((a >> (ps - 1)) & 1) a = (~a) + 1;
    if((b >> (ps - 1)) & 1) b = (~b) + 1;
    return ((int64_t)a > (int64_t)b);
}

bool ge(posit_t a, posit_t b)
{
    if((a >> (ps - 1)) & 1) a = (~a) + 1;
    if((b >> (ps - 1)) & 1) b = (~b) + 1;
    return ((int64_t)a >= (int64_t)b);
}

bool le(posit_t a, posit_t b)
{
    if((a >> (ps - 1)) & 1) a = (~a) + 1;
    if((b >> (ps - 1)) & 1) b = (~b) + 1;
    return ((int64_t)a <= (int64_t)b);
}

posit_t addition(posit_t a, posit_t b)
{
    return encode(add(decode(a), decode(b)));
}

posit_t substraction(posit_t a, posit_t b)
{
    return encode(sub(decode(a), decode(b)));
}

posit_t multiplication(posit_t a, posit_t b)
{
    return encode(mul(decode(a), decode(b)));
}

posit_t division(posit_t a, posit_t b)
{
    return encode(div(decode(a), decode(b)));
}

posit_t minus(posit_t p)
{
    unum_t d = decode(p);
    d.sign = !d.sign;
    return encode(d);
}

/*
 * Integer to Posit
 */
posit_t iTOp(int64_t a)
{
    unum_t u;
    u.sign = (a < 0);
    u.exponent = (bits(a) - 1);
    if(u.sign == 0)
    {
        u.fraction = a;
    } else 
    {
        u.fraction = (~a) + 1;
    }

    return encode(u);
}

/*
 * Float to posit
 */
posit_t fTOp(float f)
{
    uint32_t a = *((uint32_t*)(&f));

    unum_t p;
    /* Get sign bit */
    p.sign = ((a & (1 << 31)) > 0);
    /* Extract exponent */
    p.exponent = (a & mask(8, 23)) >> 23;
    p.exponent -= 127;
    /* Extract mantissa */
    p.fraction = a & mask(23, 0);
    p.fraction |= (1 << 23);
    p.fraction >>= std::max((int64_t)0, (23 - ps));

    return encode(p);
}

int64_t pTOi(posit_t p)
{
    unum_t u = decode(p);

    int64_t r = u.fraction;

    if(u.exponent < 0) return 0;
    if(u.exponent > 62) return mask(63, 0);

    int64_t b = bits(u.fraction) - 1;
    if(b > u.exponent) r = (u.fraction >> (b - u.exponent));
    if(b < u.exponent) r = (u.fraction << (u.exponent - b));

    if(u.sign)
    {
        r = (~r) + 1;
    }

    return r;
}

/*
 * NOTE: Use with care, overflows not handled!
 */
float pTOf(posit_t p)
{
    unum_t u = decode(p);

    float f;
    int32_t* fp = (int32_t*)&f;

    if(u.sign == 1)
    {
        *fp |= (1 << 31);
    }

    *fp |= (((u.exponent - 129) & mask(8, 0)) << 23);
    *fp |= (u.fraction << (23 - (bits(u.fraction) - 1))) & mask(23, 0);

    return *(float*)fp;
}

struct Posit
{
    const posit_t val;

    Posit(posit_t v) : val(v) {}
    Posit(int64_t i) : val(iTOp(i)) {}
    Posit(float f) : val(fTOp(f)) {}

    int64_t toInt()
    {
        return pTOi(this->val);
    }

    float toFloat()
    {
        return pTOf(this->val);
    }

    Posit operator+(Posit other)
    {
        return Posit(addition(this->val, other.val));
    }

    Posit operator-(Posit other)
    {
        return Posit(substraction(this->val, other.val));
    }

    Posit operator*(Posit other)
    {
        return Posit(multiplication(this->val, other.val));
    }

    Posit operator/(Posit other)
    {
        return Posit(division(this->val, other.val));
    }

    Posit operator-()
    {
        return minus(this->val);
    }

    bool operator==(Posit other)
    {
        return eq(this->val, other.val);
    }

    bool operator<(Posit other)
    {
        return lt(this->val, other.val);
    }

    bool operator>(Posit other)
    {
        return gt(this->val, other.val);
    }

    bool operator<=(Posit other)
    {
        return le(this->val, other.val);
    }

    bool operator>=(Posit other)
    {
        return ge(this->val, other.val);
    }
};
