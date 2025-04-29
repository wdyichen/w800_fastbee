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

#include <ctype.h>

#include "webnet.h"
#include "wn_utils.h"

#if (!CONFIG_WEBNET_FS)
uint32_t file_count = 0;
struct webnet_file_items *file_items = NULL;

static uint8_t *wn_ram_file = NULL;
static uint32_t wn_ram_file_size = 0;
static char    *wn_ram_file_name = NULL;
#define UPLOAD_DIR_NAME     "upload"

int wn_fs_in_ram_init(const char *filename)
{
    if (wn_ram_file_name)
        wm_os_internal_free(wn_ram_file_name);
    if (wn_ram_file)
        wm_os_internal_free(wn_ram_file);

    wn_ram_file = NULL;
    wn_ram_file_size = 0;
    wn_ram_file_name = NULL;

    wn_ram_file_name = strdup(filename);

    if (wn_ram_file_name)
        wn_ram_file = wm_os_internal_malloc(CONFIG_WEBNET_FS_IN_RAM_SIZE);
    if (wn_ram_file)
    {
        return 0;
    }
    else
    {
        if (wn_ram_file_name)
        {
            wm_os_internal_free(wn_ram_file_name);
            wn_ram_file_name = NULL;
        }
        return -1;
    }
}

void wn_fs_in_ram_deinit(void)
{
    if (wn_ram_file_name)
        wm_os_internal_free(wn_ram_file_name);
    if (wn_ram_file)
        wm_os_internal_free(wn_ram_file);

    wn_ram_file = NULL;
    wn_ram_file_size = 0;
    wn_ram_file_name = NULL;
}

int wn_fs_in_ram_write(const uint8_t *data, size_t size)
{
    size_t to_wr;

    if (!wn_ram_file) return -1;

    if ((wn_ram_file_size + size) <= CONFIG_WEBNET_FS_IN_RAM_SIZE)
        to_wr = size;
    else
        to_wr = CONFIG_WEBNET_FS_IN_RAM_SIZE - wn_ram_file_size;

    memcpy(wn_ram_file + wn_ram_file_size, data, to_wr);
    wn_ram_file_size += to_wr;

    return to_wr;
}

int wn_fs_in_ram_read(uint8_t *buf, size_t size)
{
    size_t to_rd;

    if (!wn_ram_file) return -1;

    if (wn_ram_file_size > size)
        to_rd = size;
    else
        to_rd = wn_ram_file_size;

    memcpy(buf, wn_ram_file, to_rd);

    return to_rd;
}

void wn_associated_file_information(char *file_name, const uint8_t *file_content, const uint32_t file_size)
{
    if (NULL == file_items)
    {
        file_count = 1;
        file_items = (struct webnet_file_items *)wn_malloc(sizeof(struct webnet_file_items) *file_count);
    }
    else
    {
        file_count += 1;
        file_items = (struct webnet_file_items *)wn_realloc(file_items, sizeof(struct webnet_file_items) *file_count);
    }
    file_items[file_count - 1].file_name    = file_name;
    file_items[file_count - 1].file_content = file_content;
    file_items[file_count - 1].file_size    = file_size;
}

void wn_get_file_information(char *file_name, struct webnet_file_items *items)
{
    int found = 0;
    int index = 0;

    for (index = 0; file_count > index; index ++)
    {
        if (!strcmp(file_name, file_items[index].file_name))
        {
            items->file_name    = file_items[index].file_name;
            items->file_content = file_items[index].file_content;
            items->file_size    = file_items[index].file_size;
            found = 1;
        }
    }

    if (!found)
    {
        if (0 == strcmp(file_name, wn_ram_file_name))
        {
            items->file_name    = wn_ram_file_name;
            items->file_content = wn_ram_file;
            items->file_size    = wn_ram_file_size;
        }
    }
}

char *wn_get_file_name_from_uri(const char *uri)
{
    int index = 0;
    char *root = "/index.html";
    if( 0 == strcmp(webnet_get_root(), uri))
    {
        return root;
    }
    else
    {
        for(index = 0; file_count > index; index ++){
            if(strstr(uri, file_items[index].file_name) != NULL)
            {
                break;
            }
        }
        if(file_count == index)
        {
            if (wn_ram_file_name)
            {
                char *ret = NULL;
                char *wroot = (char *)webnet_get_root();
                char *buf = wm_os_internal_malloc(strlen(wroot) + 1 + strlen(UPLOAD_DIR_NAME) + 1);
                if (buf)
                {
                    if (wroot[strlen(wroot) -1] != '/')
                        sprintf(buf, "%s//%s", wroot, UPLOAD_DIR_NAME);
                    else
                        sprintf(buf, "%s%s", wroot, UPLOAD_DIR_NAME);
                    if (0 == strcmp(buf, uri))
                        ret = wn_ram_file_name;
                    wm_os_internal_free(buf);
                    return ret;
                }
            }
            return NULL;
        }
        return file_items[index].file_name;
    }

}
#endif

wm_inline int tohex(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

int str_path_with(const char *s, const char *t)
{
    if ((strncasecmp(s, t, strlen(t)) == 0
            && (strlen(s) == strlen(t) || *(s + strlen(t)) == '/'))
        ||(strlen(t) == 1 && t[0] == '/')) return 1;

    return 0;
}

int str_begin_with(const char *s, const char *t)
{
    if (strncasecmp(s, t, strlen(t)) == 0) return 1;

    return 0;
}

int str_end_with(const char* s, const char* t)
{
    const char* se;
    register int s_len, t_len;

    s_len = strlen(s);
    t_len = strlen(t);

    if (s_len < t_len) return 0;

    se = s + s_len - t_len;
    if (strncasecmp(se, t, t_len) == 0) return 1;

    return 0;
}

char *str_decode_path(char *path)
{
    int x1;
    int x2;
    char *src = path;
    char *dst = path;
    char last = *path;

    if (last != '/')
        return NULL;

    while (*++src)
    {
        if (*src == '%' &&
                (x1 = tohex(*(src + 1))) >= 0 &&
                (x2 = tohex(*(src + 2))) >= 0)
        {
            src += 2;
            if ((*src = x1 * 16 + x2) == 0) break;
        }

        if (*src == '\\') *src = '/';
        if ((last != '.' && last != '/') || (*src != '.' && *src != '/'))
            *dst++ = last = *src;
    }

    *dst = 0;

    return path;
}

static const unsigned char base64_table[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
char *str_base64_encode(const char* src)
{
    unsigned char *out, *pos;
    const unsigned char *end, *in;
    size_t olen;
    int len;

    len = strlen(src);
    olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
    olen += olen / 72; /* line feeds */
    olen++; /* nul termination */

    out = (unsigned char*)wn_malloc(olen);
    if (out == NULL) return NULL;

    end = (const unsigned char*)src + len;
    in = (const unsigned char*)src;
    pos = out;
    while (end - in >= 3)
    {
        *pos++ = base64_table[in[0] >> 2];
        *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
        *pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
        *pos++ = base64_table[in[2] & 0x3f];
        in += 3;
    }

    if (end - in)
    {
        *pos++ = base64_table[in[0] >> 2];

        if (end - in == 1)
        {
            *pos++ = base64_table[(in[0] & 0x03) << 4];
            *pos++ = '=';
        }
        else
        {
            *pos++ = base64_table[((in[0] & 0x03) << 4) |
                                  (in[1] >> 4)];
            *pos++ = base64_table[(in[1] & 0x0f) << 2];
        }
        *pos++ = '=';
    }

    *pos = '\0';
    return (char*)out;
}

char* str_normalize_path(char* fullpath)
{
    char *dst0, *dst, *src;

    src = fullpath;
    dst = fullpath;

    dst0 = dst;
    while (1)
    {
        char c = *src;

        if (c == '.')
        {
            if (!src[1]) src ++; /* '.' and ends */
            else if (src[1] == '/')
            {
                /* './' case */
                src += 2;

                while ((*src == '/') && (*src != '\0')) src ++;
                continue;
            }
            else if (src[1] == '.')
            {
                if (!src[2])
                {
                    /* '..' and ends case */
                    src += 2;
                    goto up_one;
                }
                else if (src[2] == '/')
                {
                    /* '../' case */
                    src += 3;

                    while ((*src == '/') && (*src != '\0')) src ++;
                    goto up_one;
                }
            }
        }

        /* copy up the next '/' and erase all '/' */
        while ((c = *src++) != '\0' && c != '/') *dst ++ = c;

        if (c == '/')
        {
            *dst ++ = '/';
            while (c == '/') c = *src++;

            src --;
        }
        else if (!c) break;

        continue;

up_one:
        dst --;
        if (dst < dst0) return NULL;
        while (dst0 < dst && dst[-1] != '/') dst --;
    }

    *dst = '\0';

    /* remove '/' in the end of path if exist */
    dst --;
    if ((dst != fullpath) && (*dst == '/')) *dst = '\0';

    return fullpath;
}

char * urlencode(const char *str, int len, int *new_length)
{
    const char hexchars[] = "0123456789ABCDEF";

    const char *from, *end;
    const char *start;

    char *to;
    int c;

    from = str;
    end = str + len;
    start = to = (char *) wn_malloc(3 * len + 1);
    if(start == NULL)
    {
        return NULL;
    }

    while (from < end)
    {
        c = *from++;

        if ( (c < '0' && c != '-' && c != '.')
                 || (c == ' ')
                 || (c < 'A' && c > '9')
                 || (c > 'Z' && c < 'a' && c != '_')
                 || (c > 'z') )
        {
            to[0] = '%';
            to[1] = hexchars[c >> 4];
            to[2] = hexchars[c & 15];
            to += 3;
        }
        else
        {
            *to++ = c;
        }
    }

    *to = 0;
    if (new_length)
    {
        *new_length = to - start;
    }

    return (char *) start;
}

int urldecode(char *str, int len)
{
    char *dest = str;
    char *data = str;

    int value;
    int c;

    while (len--)
    {
        if (*data == '+')
        {
            *dest = ' ';
        }
        else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1))
                 && isxdigit((int) *(data + 2)))
        {
            c = ((unsigned char *)(data+1))[0];
            if (isupper(c))
                c = tolower(c);
            value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

            c = ((unsigned char *)(data+1))[1];
            if (isupper(c))
                c = tolower(c);
            value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

            *dest = (char)value ;
            data += 2;
            len -= 2;
        }
        else
        {
            *dest = *data;
        }
        data++;
        dest++;
    }
    *dest = '\0';

    return dest - str;
}
#ifdef _WIN32
int  strncasecmp ( const char* s1, const char* s2, size_t len )
{
    register unsigned int  x2;
    register unsigned int  x1;
    register const char*   end = s1 + len;

    while (1)
    {
        if ((s1 >= end) )
            return 0;

        x2 = *s2 - 'A'; if ((x2 < 26u)) x2 += 32;
        x1 = *s1 - 'A'; if ((x1 < 26u)) x1 += 32;
        s1++; s2++;

        if ((x2 != x1))
            break;

        if ((x1 == (unsigned int)-'A'))
            break;
    }

    return x1 - x2;
}
#endif
