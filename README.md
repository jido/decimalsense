# decimalsense
A decimal number format that makes sense

Have you ever looked at the IEEE-754 decimal number standard and wondered what in the world were they thinking?
Decimal numbers do not need be that complex.

This is an attempt at making a decimal number format similar to decimal64, but with _less complications_.

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

 * Offset by 0x0010000000000000 which increases the exponent range by 1
 * The mantissa is normalised, it goes from 1,000,000,000,000,000 to 9,999,999,999,999,999

Add 1,000,000,000,000,000 to the 53-bit number to read the actual mantissa
 
Subnormal numbers
-----------------

 Equivalent to 64-bit integers (ignoring the sign bit) from 0 to 9,999,999,999,999,999
