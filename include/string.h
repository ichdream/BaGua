/*
* 内核中的字符串操作相关函数
* 在内核是无法调用c库函数的，所以需要单独实现
* 考虑到string相关函数已经广为人知，为了方便理解和操作，我们的接口定义按照常用的方式来
* 参照了glibc相关的标准和规范:
* 1) memcpy 内存拷贝函数
* 2) memset 内存初始化函数
* 3) bzero 内存清零函数
* 4) strcmp 字符串比较函数
* 5) strcpy 字符串拷贝函数
* 6) strlen 求字符串长度函数
*
* date: 2021-08-11
* email： ichdream@foxmail.com
*/
#ifndef INCLUDE_STRING_H_
#define INCLUDE_STRING_H_

#include "types.h"

static inline void memcpy(uint8_t *dest, const uint8_t *src, uint32_t len)
{
    for (; len != 0; len--) {
        *dest++ = *src++;
    }
}

static inline void memset(void *dest, uint8_t val, uint32_t len)
{
    uint8_t *dst = (uint8_t *)dest;

    for ( ; len != 0; len--) {
        *dst++ = val;
    }
}

static inline void bzero(void *dest, uint32_t len)
{
    memset(dest, 0, len);
}

static inline int strcmp(const char *str1, const char *str2)
{
    while (*str1 && *str2 && *str1 == *str2) {
        str1++;
        str2++;
    }

    return *str1 - *str2;
}

static inline char *strcpy(char *dest, const char *src)
{
    char *tmp = dest;

    while (*src) {
          *dest++ = *src++;
    }

    *dest = '\0';

    return tmp;
}

static inline char *strcat(char *dest, const char *src)
{
    char *cp = dest;

    while (*cp) {
          cp++;
    }

    while ((*cp++ = *src++))
          ;

    return dest;
}

static inline int strlen(const char *src)
{
    const char *eos = src;

        while (*eos++)
          ;

    return (eos - src - 1);
}

#endif  // INCLUDE_STRING_H_