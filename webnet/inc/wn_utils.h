/*
 * File      : wn_utils.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2018, RT-Thread Development Team
 *
 * This software is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http://www.gnu.org/licenses/>.
 *
 * You are free to use this software under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively for commercial application, you can contact us
 * by email <business@rt-thread.com> for commercial license.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-08-02     Bernard      the first version
 */

#ifndef __WN_UTILS_H__
#define __WN_UTILS_H__

#include <string.h>

#include "wm_task_config.h"
#include "wmsdk_config.h"
#include "wm_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (!CONFIG_WEBNET_FS)
struct webnet_file_items
{
    char* file_name;
    const uint8_t* file_content;
    uint32_t file_size;

};
#endif

#ifdef __CC_ARM                         /* ARM Compiler */
    #define wm_inline                   static __inline
#elif defined (__IAR_SYSTEMS_ICC__)     /* for IAR Compiler */
    #define wm_inline                   static inline
#elif defined (__GNUC__)                /* GNU GCC Compiler */
    #define wm_inline                   static __inline
#elif defined (__ADSPBLACKFIN__)        /* for VisualDSP++ Compiler */
    #define wm_inline                   static inline
#elif defined (_MSC_VER)
    #define wm_inline                   static __inline
#elif defined (__TI_COMPILER_VERSION__)
    #define wm_inline                   static inline
#else
    #error not supported tool chain
#endif

#define WM_ALIGN_DOWN(size,align)	((size)&~((align)-1))

typedef unsigned long                   wm_ubase_t;
typedef unsigned long                   wm_ubase_t;
typedef int                             wm_bool_t;
typedef wm_ubase_t                      wm_size_t;

#if (!CONFIG_WEBNET_FS)
int wn_fs_in_ram_init(const char *filename);
void wn_fs_in_ram_deinit(void);
int wn_fs_in_ram_write(const uint8_t *data, size_t size);
int wn_fs_in_ram_read(uint8_t *buf, size_t size);
void wn_associated_file_information(char *file_name, const uint8_t* file_content, const uint32_t file_size);
void wn_get_file_information(char* file_name, struct webnet_file_items* items);
char *wn_get_file_name_from_uri(const char* uri);
#endif
int str_begin_with(const char* s, const char* t);
int str_end_with(const char* s, const char* t);
int str_path_with(const char* s, const char* t);
char* str_decode_path(char* path);
char* str_base64_encode(const char* src);
char* str_normalize_path(char* fullpath);
char* urlencode(const char* str, int len, int* new_length);
int urldecode(char* str, int len);

#ifdef  __cplusplus
    }
#endif

#endif /* __WN_UTILS_H__ */

