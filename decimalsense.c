/*
 Decimal 64 bit numbers that make sense

 Format:
 =======
 seeeeeee eeemmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm

 s = sign bit
 e = 10-bit exponent
 m = 53-bit mantissa

 Exponent is offset by 512
 If e is all 0 then it is a subnormal number
 If e is all 1 then it is an Infinity or NaN

 Normal numbers
 ==============
 The mantissa is normalised, it goes from 1,000,000,000,000,000 to 9,999,999,999,999,999
 Add 1,000,000,000,000,000 to the 53-bit number to read the actual mantissa

 Subnormal numbers
 =================
 Equivalent to 64-bit integers (ignoring the sign bit) from 0 to 999,999,999,999,999
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef uint64_t decimal;

#ifndef NON_STANDARD_SPECIAL_NUMBERS
const decimal INFINITY = 0x7ff0000000000000L;
const decimal NAN =      0x7ff8000000000001L;
const int EXPONENT_MAX = 510;
#else
const decimal INFINITY = 0x7ffffc0000000000L;
const decimal NAN =      0x7ffffe0000000001L;
const int EXPONENT_MAX = 511;
#endif
const int EXPONENT_OFFSET = 512;
const int EXPONENT_SHIFT = 53;
const decimal EXPONENT_MASK = 0x3ffL;
#ifdef ENABLE_SUBNORMAL_NUMBERS
const int EXPONENT_MIN = -511;
#else
const int EXPONENT_MIN = -512;
#endif
const decimal SIGNIFICAND_OFFSET = 1000000000000000L;
const decimal SIGNIFICAND_MASK = 0x001fffffffffffffL;
const decimal SIGNIFICAND_MAX =    9999999999999999L;
const decimal UNIT_DIGIT =         1000000000000000L;
const int SIGN_SHIFT = 63;
const decimal SIGN_BIT = 1L << SIGN_SHIFT;

char * numberAsString(decimal num) {
    static char result[] = "+1.234567890123456e-123";

    char sign = (num & SIGN_BIT) ? '-' : '+';
    
    // Starting as subnormal
    int16_t expn = EXPONENT_MIN;
    uint64_t positive = num & ~SIGN_BIT;
    if (positive == INFINITY)
    {
        static char inf[] = "+Infinity";
        inf[0] = sign;
        return inf;
    }
    if (positive > INFINITY)
    {
        return "NaN";
    }
#ifdef ENABLE_SUBNORMAL_NUMBERS
    if (positive >= (1L << EXPONENT_SHIFT))
    {
        // Not a subnormal number
#else
    if (positive != 0)
    {
#endif
        expn = (positive >> EXPONENT_SHIFT) - EXPONENT_OFFSET;
        positive = (positive & SIGNIFICAND_MASK) + SIGNIFICAND_OFFSET;
        if (positive > SIGNIFICAND_MAX
#ifndef ENABLE_SUBNORMAL_NUMBERS
            || positive < UNIT_DIGIT
#endif
            )
        {
            return "NaN";
        }
#ifdef ENABLE_SUBNORMAL_NUMBERS
    }
    else if (positive > SIGNIFICAND_MAX / 10)
    {
        return "NaN";
#endif
    }
    
    result[0] = sign;
    sprintf(result + 2, "%0.16llu", positive);
    result[1] = result[2];
    result[2] = '.';
    result[18] = 'e';
    sprintf(result + 19, "%+d", expn);
    return result;
}

decimal makeNumber_(uint64_t negat, uint64_t decimals, uint64_t expn) {
    decimal result = negat << SIGN_SHIFT;
#ifdef ENABLE_SUBNORMAL_NUMBERS
    if (expn == 0) return result | decimals;
#endif
    result |= expn << EXPONENT_SHIFT;
#ifndef ENABLE_SUBNORMAL_NUMBERS
    // Clip to zero if value < 1.0e-512 (assuming expn == 0)
    if (decimals > SIGNIFICAND_OFFSET)
#endif
        result |= decimals - SIGNIFICAND_OFFSET;
    return result;
}

decimal makeNumber(int units, uint64_t decimals, int expn) {
    return makeNumber_(units < 0, decimals + abs(units) * UNIT_DIGIT, expn + EXPONENT_OFFSET);
}

int numberParts_(decimal num, int * expn, uint64_t * decimals)
{
    *expn = (num >> EXPONENT_SHIFT) & EXPONENT_MASK;
    *decimals = num & SIGNIFICAND_MASK;
#ifdef ENABLE_SUBNORMAL_NUMBERS
    if (*expn > 0)
    {
        // Normal number
#endif
        *decimals += SIGNIFICAND_OFFSET;
#ifdef ENABLE_SUBNORMAL_NUMBERS
    }
#endif
    return num >> SIGN_SHIFT;   // sign bit
}

uint64_t shiftDecimals(uint64_t decimals, int amount) {
    if (amount < -15 || decimals == 0)
    {
        return 0;
    }
    else if (amount < 0)
    {
        switch ((-amount) % 4)
        {
            case 1:
            decimals /= 10;
            break;
            
            case 2:
            decimals /= 100;
            break;
            
            case 3:
            decimals /= 1000;
            break;
        }
        amount = amount / 4 * 4;
        for (int t = 0; t > amount && decimals != 0; t -= 4)
        {
            decimals /= 10000;
        }
        return decimals;
    }
    else
    {
        uint32_t fac = 1;
        for (int h = 0; h < amount; ++h)
        {
            fac *= 10;
        }
        return decimals * fac;
    }
}

decimal add(decimal a, decimal b) {
    int exp_a, exp_b;
    uint64_t m_a, m_b;
    int sign_a = numberParts_(a, &exp_a, &m_a);
    int sign_b = numberParts_(b, &exp_b, &m_b);
    int expn = (exp_a > exp_b) ? exp_a : exp_b;
    uint64_t sign = 0;
    uint64_t sum;
    if (exp_a == exp_b || expn == 1)
    {
        // Same exponent
        if ((sign_a == 1 && m_a > m_b) || (sign_b == 1 && m_b > m_a))
        {
            sign = 1;
        }
        sum = (sign_a == sign_b) ? m_a + m_b : llabs((int64_t) m_a - (int64_t) m_b);
    }
    else
    {
        // Different exponents
        if ((sign_a == 1 && exp_a == expn) || (sign_b == 1 && exp_b == expn))
        {
            sign = 1;
        }
        int exp_diff = abs(exp_b - exp_a);
        uint64_t large = (exp_a == expn) ? m_a : m_b;
        uint64_t small = (exp_a == expn) ? m_b : m_a;
        if (sign_a != sign_b && expn > 1 && large < 2 * UNIT_DIGIT - 1)                                
        {
            // Sum could lose precision
            large *= 10;
            --expn;
            --exp_diff;
        }
        small = shiftDecimals(small, -exp_diff);
        sum = (sign_a == sign_b) ? large + small : llabs((int64_t) large - (int64_t) small);
    }
    if (sum > SIGNIFICAND_MAX)
    {
        sum /= 10;
        ++expn;
    }
    while (expn > 0 && sum < UNIT_DIGIT)
    {
#ifdef ENABLE_SUBNORMAL_NUMBERS
        if (expn > 1)
#endif
            sum *= 10;
        --expn;
    }
    return makeNumber_(sign, sum, expn);
}

decimal opp(decimal num) {
    return num ^ SIGN_BIT;
}

decimal sub(decimal a, decimal b) {
    return add(a, opp(b));
}

typedef unsigned __int128 uint128_t;

decimal mul(decimal a, decimal b) {
    int exp_a, exp_b;
    uint64_t m_a, m_b;
    int sign_a = numberParts_(a, &exp_a, &m_a);
    int sign_b = numberParts_(b, &exp_b, &m_b);
    uint64_t negat = (sign_a != sign_b);
    int expn = exp_a + exp_b - EXPONENT_OFFSET;
#ifdef ENABLE_SUBNORMAL_NUMBERS
    if (exp_a == 0 || exp_b == 0)
    {
        // Subnormal exponent is -511 instead of -512
        expn += 1;
    }
#endif
    if (expn < 0)
    {
        // Too small, return zero
        return negat << SIGN_SHIFT;
    }
    else if (expn > EXPONENT_OFFSET + EXPONENT_MAX)
    {
        // Too large, return infinity
        return INFINITY | (negat << SIGN_SHIFT);
    }
    else
    {
        uint64_t prod = ((uint128_t) m_a * m_b) / UNIT_DIGIT;
        if (prod > SIGNIFICAND_MAX)
        {
            prod /= 10;
            expn += 1;
        }
        while (expn > 0 && prod < UNIT_DIGIT)
        {
#ifdef ENABLE_SUBNORMAL_NUMBERS
            if (expn > 1)
#endif
                prod *= 10;
            --expn;
        }
        return makeNumber_(negat, prod, expn);
    }
}

decimal divs(decimal a, decimal b) {
    int exp_a, exp_b;
    uint64_t m_a, m_b;
    int sign_a = numberParts_(a, &exp_a, &m_a);
    int sign_b = numberParts_(b, &exp_b, &m_b);
    uint64_t negat = (sign_a != sign_b);
    int expn = exp_a - exp_b + EXPONENT_OFFSET;
#ifdef ENABLE_SUBNORMAL_NUMBERS
    if (exp_a == 0) expn += 1;      // subnormal exponent is -511
    if (exp_b == 0) expn -= 1;      // instead of -512, so adjust it
#endif
    uint128_t quo = ((uint128_t) m_a * UNIT_DIGIT) / m_b;
    while (expn < 0 || quo > SIGNIFICAND_MAX)
    {
        if (expn >= EXPONENT_OFFSET + EXPONENT_MAX)
        {
            // Too large, return infinity
            return INFINITY | (negat << SIGN_SHIFT);
        }
        if (quo == 0)
        {
            return negat << SIGN_SHIFT;
        }
        quo /= 10;
        expn += 1;
    }
    while (expn > 0 && quo < UNIT_DIGIT)
    {
#ifdef ENABLE_SUBNORMAL_NUMBERS
        if (expn > 1)
#endif
            quo *= 10;
        --expn;
    }
    return makeNumber_(negat, quo, expn);
}

int main(int n, char * args[]) {
#ifdef NON_STANDARD_SPECIAL_NUMBERS
    puts(numberAsString(0x7ffFF973CAFA7FFFL));  // Largest number 9.99...e+511 (non-standard)
#else
    puts(numberAsString(0x7fdFF973CAFA7FFFL));  // Largest number 9.99...e+510 (standard)
#endif
#ifdef ENABLE_SUBNORMAL_NUMBERS
    puts(numberAsString(0x0020000000000000L));  // Smallest normal number 1.0e-511
    puts(numberAsString(0x00038D7EA4C67FFFL));  // Largest subnormal number 9.99...e-512
#endif
    puts(numberAsString(0x0000000000000001L));  // Smallest number 1.0e-526
    puts(numberAsString(makeNumber(+4, 698543100238333L, +10)));
    puts(numberAsString(9000000000000001L));    // Not a number (over 15 digits)
    puts(numberAsString(INFINITY));  // +∞
    puts(numberAsString(SIGN_BIT | INFINITY));  // -∞
    printf("Zero: 0x%.16llx \n", makeNumber(0, 0, -EXPONENT_OFFSET));        // 0
    printf("One: 0x%.16llx \n", makeNumber(+1, 0, 0));           // 0x4000000000000000
    printf("Ten: 0x%.16llx \n", makeNumber(+1, 0, +1));          // 0x4020000000000000
    printf("Minus five dot five: 0x%.16llx \n", makeNumber(-5, 500000000000000L, 0));    // 0xc00ffcb9e57d4000
    puts(numberAsString(add(makeNumber(+1, 0, EXPONENT_MIN), 1234567)));
    puts(numberAsString(add(makeNumber(+1, 9, 0), makeNumber(+1, 4, 0))));
    puts(numberAsString(add(makeNumber(-1, 0, EXPONENT_MIN), 1234567)));
    puts(numberAsString(sub(makeNumber(1, 0, 0), makeNumber(1, 234567890000000L, -10))));
    puts(numberAsString(sub(makeNumber(+1, 9, 105), makeNumber(+1, 4, 105))));
    puts(numberAsString(mul(makeNumber(3, 0, +3), makeNumber(5, 0, 0))));
    puts(numberAsString(mul(makeNumber(-1, 0, EXPONENT_MIN), 1234567)));
    puts(numberAsString(mul(makeNumber(-1, 0, +1), 1234567)));
    puts(numberAsString(divs(makeNumber(3, 0, +3), makeNumber(5, 0, 0))));
    puts(numberAsString(divs(makeNumber(-1, 0, EXPONENT_MIN), 1234567)));
    puts(numberAsString(divs(makeNumber(-1, 0, -8), 1234567)));
    puts(numberAsString(divs(makeNumber(3, 0, EXPONENT_MIN), makeNumber(4, 0, 400))));
}
