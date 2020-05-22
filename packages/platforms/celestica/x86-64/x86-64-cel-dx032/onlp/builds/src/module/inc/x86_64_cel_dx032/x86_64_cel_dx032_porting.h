/**************************************************************************//**
 *
 * @file
 * @brief x86_64_cel_dx032 Porting Macros.
 *
 * @addtogroup x86_64_cel_dx032-porting
 * @{
 *
 *****************************************************************************/
#ifndef __X86_64_CEL_DX032_PORTING_H__
#define __X86_64_CEL_DX032_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if X86_64_CEL_DX032_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef X86_64_CEL_DX032_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define X86_64_CEL_DX032_MALLOC GLOBAL_MALLOC
    #elif X86_64_CEL_DX032_CONFIG_PORTING_STDLIB == 1
        #define X86_64_CEL_DX032_MALLOC malloc
    #else
        #error The macro X86_64_CEL_DX032_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_CEL_DX032_FREE
    #if defined(GLOBAL_FREE)
        #define X86_64_CEL_DX032_FREE GLOBAL_FREE
    #elif X86_64_CEL_DX032_CONFIG_PORTING_STDLIB == 1
        #define X86_64_CEL_DX032_FREE free
    #else
        #error The macro X86_64_CEL_DX032_FREE is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_CEL_DX032_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define X86_64_CEL_DX032_MEMSET GLOBAL_MEMSET
    #elif X86_64_CEL_DX032_CONFIG_PORTING_STDLIB == 1
        #define X86_64_CEL_DX032_MEMSET memset
    #else
        #error The macro X86_64_CEL_DX032_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_CEL_DX032_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define X86_64_CEL_DX032_MEMCPY GLOBAL_MEMCPY
    #elif X86_64_CEL_DX032_CONFIG_PORTING_STDLIB == 1
        #define X86_64_CEL_DX032_MEMCPY memcpy
    #else
        #error The macro X86_64_CEL_DX032_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_CEL_DX032_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define X86_64_CEL_DX032_STRNCPY GLOBAL_STRNCPY
    #elif X86_64_CEL_DX032_CONFIG_PORTING_STDLIB == 1
        #define X86_64_CEL_DX032_STRNCPY strncpy
    #else
        #error The macro X86_64_CEL_DX032_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_CEL_DX032_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define X86_64_CEL_DX032_VSNPRINTF GLOBAL_VSNPRINTF
    #elif X86_64_CEL_DX032_CONFIG_PORTING_STDLIB == 1
        #define X86_64_CEL_DX032_VSNPRINTF vsnprintf
    #else
        #error The macro X86_64_CEL_DX032_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_CEL_DX032_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define X86_64_CEL_DX032_SNPRINTF GLOBAL_SNPRINTF
    #elif X86_64_CEL_DX032_CONFIG_PORTING_STDLIB == 1
        #define X86_64_CEL_DX032_SNPRINTF snprintf
    #else
        #error The macro X86_64_CEL_DX032_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_CEL_DX032_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define X86_64_CEL_DX032_STRLEN GLOBAL_STRLEN
    #elif X86_64_CEL_DX032_CONFIG_PORTING_STDLIB == 1
        #define X86_64_CEL_DX032_STRLEN strlen
    #else
        #error The macro X86_64_CEL_DX032_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __X86_64_CEL_DX032_PORTING_H__ */
/* @} */
