#ifndef __PTR2INT_H__
#define __PTR2INT_H__

#if __SIZEOF_POINTER__ == 8
typedef long ptr_int_cast;
#elif __SIZEOF_POINTER__ == 4
typedef int ptr_int_cast;
#else
#error "unsupported pointer size"
#endif

#endif
