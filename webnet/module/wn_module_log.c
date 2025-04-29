/*
 * File      : wn_module_log.c
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
#include "webnet.h"
#include "wn_module.h"
#include "wn_utils.h"
#include "lwip/inet.h"

#define LOG_TAG "webnet"
#include "wm_log.h"

int webnet_module_log(struct webnet_session* session, int event)
{
    struct webnet_request* request;

    if (session != NULL)
    {
        request = session->request;
    }
    else
    {
        request = NULL;
    }

    if (event == WEBNET_EVENT_INIT)
    {
        wm_log_debug("server initialize success.");
    }
    else if (event == WEBNET_EVENT_URI_PHYSICAL)
    {
        uint32_t index;
        wm_log_raw("\n");
        wm_log_debug("  new client: %s:%u",
                   inet_ntoa(session->cliaddr.sin_addr),
                   ntohs(session->cliaddr.sin_port));

        switch (request->method)
        {
        case WEBNET_GET:
            wm_log_debug("      method: GET");
            break;
        case WEBNET_PUT:
            wm_log_debug("      method: PUT");
            break;
        case WEBNET_POST:
            wm_log_debug("      method: POST");
            break;
        case WEBNET_HEADER:
            wm_log_debug("      method: HEADER");
            break;
        case WEBNET_SUBSCRIBE:
            wm_log_debug("      method: SUBSCRIBE");
            break;
        case WEBNET_UNSUBSCRIBE:
            wm_log_debug("      method: UNSUBSCRIBE");
            break;
        default:
            break;
        }
        wm_log_debug("     request: %s", request->path);
        for (index = 0; index < request->query_counter; index ++)
        {
            wm_log_debug("    query[%d]: %s => %s", index,
                       request->query_items[index].name,
                       request->query_items[index].value);
        }
    }
    else if (event == WEBNET_EVENT_URI_POST)
    {
        wm_log_debug("physical url: %s", request->path);
        wm_log_debug("   mime type: %s", mime_get_type(request->path));
    }
    else if (event == WEBNET_EVENT_RSP_HEADER)
    {
    }
    else if (event == WEBNET_EVENT_RSP_FILE)
    {
    }

    return WEBNET_MODULE_CONTINUE;
}
