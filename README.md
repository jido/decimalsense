# decimalsense
A decimal number format that makes sense

Have you ever looked at the IEEE-754 decimal number standard and wondered what in the world were they thinking?
Decimal numbers do not need be that complex.

This is an attempt at making a decimal number format similar to decimal64, but with _less complications_.

Numbers
=======

Normal number range:

**1.0e-510** to **9.999,999,999,999,999e+511**

(hexadecimal _300000 00000000_ to _7FEFF973 CAFA7FFF_)

Subnormal number range (non-zero):

**1.0e-526** to **9.999,999,999,999,999e-511**

(hexadecimal _1_ to _2386F2 6FC0FFFF_)

Zero:

> hexadecimal _0_

Negative zero:

> hexadecimal _80000000 00000000_

One:

> hexadecimal _3FF00000 00000000_

Ten:

> hexadecimal _40100000 00000000_

Infinity:

> hexadecimal _7FF00000 00000000_

Negative infinity:

> hexadecimal _FFF00000 00000000_

Decimalsense numbers are _monotonic_, the unsigned part is ordered when cast as integer, and _unique_: 
there is only one way to represent a particular number. Not a Number (NaN) can have several representations.

Format
======

~~~
seeeeeee eepmmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm
~~~

   `s` = sign bit
   
   `e` = 10-bit exponent (with `p`)
   
   `p` = lowest bit of exponent or highest bit of mantissa (subnormal numbers)
   
   `m` = 53-bit mantissa
   
Exponent is offset by 511

 * If `e` is all 0 then it is a subnormal number unless `p` = 1 and leftmost `m` bit = 1
 * If `e` is all 1 then it is an _Infinity_ or _NaN_ unless `p` = 0 or leftmost `m` bit = 0
 
Normal numbers
--------------

 * Offset by _hexadecimal 100000 00000000_ which increases the exponent range by 1
 * The mantissa is normalised, it goes from _1,000,000,000,000,000_ to _9,999,999,999,999,999_

Add _1,000,000,000,000,000_ to the 53-bit number to read the actual mantissa
 
Subnormal numbers
-----------------

 Equivalent to 64-bit integers (ignoring the sign bit) from _0_ to _9,999,999,999,999,999_
