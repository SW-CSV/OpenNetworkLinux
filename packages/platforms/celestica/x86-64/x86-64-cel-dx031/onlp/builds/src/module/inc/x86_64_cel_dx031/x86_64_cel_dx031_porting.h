/**************************************************************************//**
 *
 * @file
 * @brief x86_64_cel_dx031 Porting Macros.
 *
 * @addtogroup x86_64_cel_dx031-porting
 * @{
 *
 *****************************************************************************/
#ifndef __X86_64_CEL_DX031_PORTING_H__
#define __X86_64_CEL_DX031_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if X86_64_CEL_DX031_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef X86_64_CEL_DX031_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define X86_64_CEL_DX031_MALLOC GLOBAL_MALLOC
    #elif X86_64_CEL_DX031_CONFIG_PORTING_STDLIB == 1
        #define X86_64_CEL_DX031_MALLOC malloc
    #else
        #error The macro X86_64_CEL_DX031_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_CEL_DX031_FREE
    #if defined(GLOBAL_FREE)
        #define X86_64_CEL_DX031_FREE GLOBAL_FREE
    #elif X86_64_CEL_DX031_CONFIG_PORTING_STDLIB == 1
        #define X86_64_CEL_DX031_FREE free
    #else
        #error The macro X86_64_CEL_DX031_FREE is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_CEL_DX031_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define X86_64_CEL_DX031_MEMSET GLOBAL_MEMSET
    #elif X86_64_CEL_DX031_CONFIG_PORTING_STDLIB == 1
        #define X86_64_CEL_DX031_MEMSET memset
    #else
        #error The macro X86_64_CEL_DX031_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_CEL_DX031_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define X86_64_CEL_DX031_MEMCPY GLOBAL_MEMCPY
    #elif X86_64_CEL_DX031_CONFIG_PORTING_STDLIB == 1
        #define X86_64_CEL_DX031_MEMCPY memcpy
    #else
        #error The macro X86_64_CEL_DX031_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_CEL_DX031_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define X86_64_CEL_DX031_STRNCPY GLOBAL_STRNCPY
    #elif X86_64_CEL_DX031_CONFIG_PORTING_STDLIB == 1
        #define X86_64_CEL_DX031_STRNCPY strncpy
    #else
        #error The macro X86_64_CEL_DX031_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_CEL_DX031_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define X86_64_CEL_DX031_VSNPRINTF GLOBAL_VSNPRINTF
    #elif X86_64_CEL_DX031_CONFIG_PORTING_STDLIB == 1
        #define X86_64_CEL_DX031_VSNPRINTF vsnprintf
    #else
        #error The macro X86_64_CEL_DX031_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_CEL_DX031_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define X86_64_CEL_DX031_SNPRINTF GLOBAL_SNPRINTF
    #elif X86_64_CEL_DX031_CONFIG_PORTING_STDLIB == 1
        #define X86_64_CEL_DX031_SNPRINTF snprintf
    #else
        #error The macro X86_64_CEL_DX031_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_CEL_DX031_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define X86_64_CEL_DX031_STRLEN GLOBAL_STRLEN
    #elif X86_64_CEL_DX031_CONFIG_PORTING_STDLIB == 1
        #define X86_64_CEL_DX031_STRLEN strlen
    #else
        #error The macro X86_64_CEL_DX031_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __X86_64_CEL_DX031_PORTING_H__ */
/* @} */
