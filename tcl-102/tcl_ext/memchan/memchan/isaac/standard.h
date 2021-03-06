/*
------------------------------------------------------------------------------
Standard definitions and types, Bob Jenkins

Modified by Pat Thoyts for MSVC++ compatability.
------------------------------------------------------------------------------
*/
#ifndef STANDARD
# define STANDARD
# ifndef STDIO
#  include <stdio.h>
#  define STDIO
# endif
# ifndef STDDEF
#  include <stddef.h>
#  define STDDEF
# endif
#if (_MSC_VER >= 1000)
typedef  unsigned __int64  ub8;
typedef    signed __int64  sb8;
typedef  unsigned __int32  ub4;
typedef    signed __int32  sb4;
typedef  unsigned __int16  ub2;
typedef    signed __int16  sb2;
typedef  unsigned __int8   ub1;
typedef    signed __int8   sb1;
#else
typedef  unsigned long long  ub8;
typedef    signed long long  sb8;
typedef  unsigned long  int  ub4;
typedef    signed long  int  sb4;
typedef  unsigned short int  ub2;
typedef    signed short int  sb2;
typedef  unsigned       char ub1;
typedef    signed       char sb1;
#endif

#define UB8MAXVAL 0xffffffffffffffffLL
#define UB8BITS 64
#define SB8MAXVAL 0x7fffffffffffffffLL
#define UB4MAXVAL 0xffffffff
#define UB4BITS 32
#define SB4MAXVAL 0x7fffffff
#define UB2MAXVAL 0xffff
#define UB2BITS 16
#define SB2MAXVAL 0x7fff
#define UB1MAXVAL 0xff
#define UB1BITS 8
#define SB1MAXVAL 0x7f

typedef                 int  word;  /* fastest type available */

#define bis(target,mask)  ((target) |=  (mask))
#define bic(target,mask)  ((target) &= ~(mask))
#define bit(target,mask)  ((target) &   (mask))
#ifndef min
# define min(a,b) (((a)<(b)) ? (a) : (b))
#endif /* min */
#ifndef max
# define max(a,b) (((a)<(b)) ? (b) : (a))
#endif /* max */
#ifndef align
# define align(a) (((ub4)a+(sizeof(void *)-1))&(~(sizeof(void *)-1)))
#endif /* align */
#ifndef abs
# define abs(a)   (((a)>0) ? (a) : -(a))
#endif
#define TRUE  1
#define FALSE 0
#define SUCCESS 0  /* 1 on VAX */

#endif /* STANDARD */
