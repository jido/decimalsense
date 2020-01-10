# decimalsense
A decimal number format that makes sense

Have you ever looked at the IEEE-754 decimal number standard and wondered what in the world were they thinking?
Decimal numbers do not need be that complex.

This is an attempt at making a decimal number format similar to decimal64, but with _less complications_.

Numbers
=======

Normal number range:

**1.0e-511** to **9.999,999,999,999,999e+510**

(hexadecimal _200000 00000000_ to _7FDFF973 CAFA7FFF_)

Subnormal number range (non-zero):

**1.0e-526** to **9.999,999,999,999,99e-512**

(hexadecimal _1_ to _38D7E A4C67FFF_)

Zero:

> hexadecimal _0_

Negative zero:

> hexadecimal _80000000 00000000_

One:

> hexadecimal _40000000 00000000_

Ten:

> hexadecimal _40200000 00000000_

Infinity:

> hexadecimal _7FF00000 00000000_

Negative infinity:

> hexadecimal _FFF00000 00000000_

Decimalsense numbers are _monotonic_, the unsigned part is ordered when cast as integer, and _unique_: 
there is only one way to encode a particular nonzero number.
Not a Number (NaN) can have several representations.

According to IEEE, positive and negative zero are equal which is the only exception to unique number representation.

Format
======

~~~
seeeeeee eeemmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm
~~~

   `s` = sign bit
   
   `e` = 10-bit exponent
   
   `m` = 53-bit mantissa

 * If `e` is all 0 then it is a subnormal number
 * If `e` is all 1 then it is an _Infinity_ or _NaN_
 
Normal numbers
--------------

 * Exponent is offset by 512
 * The mantissa is normalised, it goes from _1,000,000,000,000,000_ to _9,999,999,999,999,999_

Take _512_ away from the 10-bit exponent value to read the actual exponent;

Add _1,000,000,000,000,000_ to the 53-bit number to read the actual mantissa

Subnormal numbers
-----------------

Equivalent to 64-bit integers (ignoring the sign bit) from _0_ to _999,999,999,999,999_;

Their exponent is _-511_ and there is no value to add to the mantissa, as it only encodes lower precision numbers between 0.0 and 1.0
