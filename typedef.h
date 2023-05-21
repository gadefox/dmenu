/* See LICENSE file for copyright and license details. */

#ifndef TYPEDEF_H
#define TYPEDEF_H


#define countof(a)  (sizeof (a) / sizeof ((a)[0]))

#define MAX(a, b)   ((a) > (b) ? (a) : (b))
#define MIN(a, b)   ((a) < (b) ? (a) : (b))

#define BETWEEN(x, a, b)   ((x) >= (a) && (x) <= (b))
#define IS_INDEX(x, n)     ((x) >= (0) && (x) < (n))

#define MAX_NUM_SIZE  (countof ("18446744073709551615"))

#ifndef True
#define True  1
#endif

#ifndef False
#define False  0
#endif


typedef unsigned int  uint;
typedef unsigned char byte;


#endif  /* _TYPEDEF_H_ */
