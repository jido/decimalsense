/*
 Decimal 64 bit numbers that make sense

 Format:
 =======
 seeeeeee eepmmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm

 s = sign bit
 e = 10-bit exponent (with p)
 p = lowest bit of exponent or highest bit of mantissa (subnormal numbers)
 m = 53-bit mantissa

 Exponent is offset by 511
 If e is all 0 then it is a subnormal number unless p = 1 and leftmost m bit = 1
 If e is all 1 then it is an Infinity or NaN unless p = 0 or leftmost m bit = 0

 Normal numbers
 ==============
 Offset by 0x0010000000000000 which increases the exponent range by 1
 The mantissa is normalised, it goes from 1,000,000,000,000,000 to 9,999,999,999,999,999
 Add 1,000,000,000,000,000 to the 53-bit number to read the actual mantissa

 Subnormal numbers
 =================
 Equivalent to 64-bit integers (ignoring the sign bit) from 0 to 9,999,999,999,999,999
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef uint64_t decimal;

char * numberAsString(decimal num) {
    static char result[] = "+1.234567890123456e-123";

    char sign = (num & 0x8000000000000000L) ? '-' : '+';
    
    // Starting as subnormal
    int16_t expn = -511;
    uint64_t positive = num & 0x7fffffffffffffffL;
    if (positive == 0x7ff0000000000000L)
    {
        static char inf[] = "+Infinity";
        inf[0] = sign;
        return inf;
    }
    if (positive > 0x7ff0000000000000L)
    {
        return "NaN";
    }
    if (positive >= 0x0030000000000000L)
    {
        // Not a subnormal number
        positive -= 0x0010000000000000L;    // Shift the range half-way
        expn += positive >> 53;
        positive = (positive & 0x001fffffffffffffL) + 1000000000000000L;
    }
    if (positive > 9999999999999999L)
    {
        return "NaN";
    }
    
    result[0] = sign;
    sprintf(result + 1, "%0.16llu", positive);
    memmove(result + 3, result + 2, 15);
    result[2] = '.';
    sprintf(result + 19, "%+d", expn);
    return result;
}

decimal makeNumber_(uint64_t negat, uint64_t decimals, uint64_t expn) {
    decimal result = negat << 63;
    if (expn == 0) return result | decimals;
    result |= expn << 53;
    result |= decimals - 1000000000000000L;
    return result + 0x0010000000000000L;
}

decimal makeNumber(int units, uint64_t decimals, int expn) {
    return makeNumber_(units < 0, decimals + abs(units) * 1000000000000000L, expn + 511);
}

int numberParts_(decimal num, int * expn, uint64_t * decimals)
{
    int e = (num >> 52) & 0x7ff;
    if (e <= 2)
    {
        // Subnormal number
        *expn = 0;
        *decimals = num & 0x003fffffffffffffL;
    }
    else
    {
        // Normal number
        *expn = (e - 1) / 2;
        *decimals = 1000000000000000L + ((num - 0x0010000000000000L) & 0x001fffffffffffffL);
    }
    return num >> 63;   // sign bit
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
    if (exp_a == exp_b)
    {
        // Same exponent
        if (sign_a != sign_b && ((sign_a == 1 && m_a > m_b) || (sign_b == 1 && m_b > m_a)))
        {
            sign = 1;
        }
        sum = (sign_a == sign_b) ? m_a + m_b : llabs((int64_t) m_a - (int64_t) m_b);
    }
    else
    {
        // Different exponents
        if (sign_a != sign_b && ((sign_a == 1 && exp_a == expn) || (sign_b == 1 && exp_b == expn)))
        {
            sign = 1;
        }
        int exp_diff = abs(exp_b - exp_a);
        uint64_t large = (exp_a == expn) ? m_a : m_b;
        uint64_t small = (exp_a == expn) ? m_b : m_a;
        if (sign_a != sign_b && large < 2000000000000000L)
        {
            // Sum could lose precision
            large *= 10;
            --expn;
            --exp_diff;
        }
        small = shiftDecimals(small, -exp_diff);
        sum = (sign_a == sign_b) ? large + small : llabs((int64_t) large - (int64_t) small);
    }
    if (sum > 9999999999999999L)
    {
        sum /= 10;
        ++expn;
    }
    while (expn != 0 && sum < 1000000000000000L)
    {
        sum *= 10;
        --expn;
    }
    return makeNumber_(sign, sum, expn);
}

decimal opp(decimal num) {
    return num ^ (1L << 63);
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
    int expn = exp_a + exp_b - 511;
    if (expn < 0)
    {
        // Too small, return zero
        return negat << 63;
    }
    else if (expn > 1022)
    {
        // Too large, return infinity
        return 0x7ff0000000000000L | (negat << 63);
    }
    else
    {
        uint64_t prod = ((uint128_t) m_a * m_b) / 1000000000000000L;
        if (prod > 9999999999999999L)
        {
            prod /= 10;
            expn += 1;
        }
        while (expn != 0 && prod < 1000000000000000L)
        {
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
    int expn = exp_a - exp_b + 510;
    uint128_t quo = ((uint128_t) m_a * 10000000000000000L) / m_b;
    while (quo > 9999999999999999L) // note: should only loop if b is subnormal
    {
        if (expn == 1022)
        {
            // Too large, return infinity
            return 0x7ff0000000000000L | (negat << 63);
        }
        quo /= 10;
        expn += 1;
    }
    while (quo < 1000000000000000L)
    {
        if (expn == 0) break;       // subnormal number
        quo *= 10;
        --expn;
    }
    return makeNumber_(negat, quo, expn);
}

int main(int n, char * args[]) {
    puts(numberAsString(0x7feFF973CAFA7FFFL));  // Largest number 9.99...e+511
    puts(numberAsString(0x0030000000000000L));  // Smallest normal number 1.0e-510
    puts(numberAsString(0x002386F26FC0FFFFL));  // Largest subnormal number 9.99...e-511
    puts(numberAsString(0x0000000000000001L));  // Smallest number 1.0e-526
    puts(numberAsString(makeNumber(+4, 698543100238333L, +10)));
    puts(numberAsString(10000000000000000L));   // Not a number (over 16 digits)
    puts(numberAsString(0x7ff0000000000000L));      // +∞
    puts(numberAsString(makeNumber(-1, 0, +512)));  // -∞
    printf("Zero: 0x%.16llx \n", makeNumber(0, 0, -511));        // 0
    printf("One: 0x%.16llx \n", makeNumber(+1, 0, 0));           // 0x3ff0000000000000
    printf("Ten: 0x%.16llx \n", makeNumber(+1, 0, +1));          // 0x4010000000000000
    printf("Minus five dot five: 0x%.16llx \n", makeNumber(-5, 500000000000000L, 0));    // 0xbffffcb9e57d4000
    puts(numberAsString(add(makeNumber(+1, 0, -510), 1234567)));
    puts(numberAsString(add(makeNumber(+1, 9, 0), makeNumber(+1, 4, 0))));
    puts(numberAsString(add(makeNumber(-1, 0, -510), 1234567)));
    puts(numberAsString(sub(makeNumber(-1, 0, 0), makeNumber(-1, 234567890000000L, -10))));
    puts(numberAsString(sub(makeNumber(+1, 9, 105), makeNumber(+1, 4, 105))));
    puts(numberAsString(mul(makeNumber(3, 0, +3), makeNumber(5, 0, 0))));
    puts(numberAsString(mul(makeNumber(-1, 0, -510), 1234567)));
    puts(numberAsString(mul(makeNumber(-1, 0, +1), 1234567)));
    puts(numberAsString(divs(makeNumber(3, 0, +3), makeNumber(5, 0, 0))));
    puts(numberAsString(divs(makeNumber(-1, 0, -510), 1234567)));
    puts(numberAsString(divs(makeNumber(-1, 0, +1), 1234567)));
}