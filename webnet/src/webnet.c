/*
 * File      : webnet.c
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
 * 2012-07-03     Bernard      Add webnet port and webroot setting interface.
 */

#include <stdint.h>
#include <string.h>

#include "webnet.h"
#include "wm_osal.h"
#include "wn_module.h"
#include "lwip/sockets.h"

#define LOG_TAG "webnet"
#include "wm_log.h"

#if defined(CONFIG_WM_NETIF_SELECT_LWIP) && (WM_TASK_TCPIP_STACK < 1408)
#error The lwIP tcpip thread stack size(RT_LWIP_TCPTHREAD_STACKSIZE) must more than 1408
#endif

#define RECEIVE_TIMEOUT   50000
#define CLOSE_TIMEOUT_MS  10

uint16_t webnet_port = CONFIG_WEBNET_PORT;

#ifdef CONFIG_WEBNET_ROOT
static char webnet_root[64] = CONFIG_WEBNET_ROOT;
#else
static char webnet_root[64] = "/";
#endif

static wm_os_task_t  webnet_task = NULL;
static int close_flag = 0;

void webnet_set_port(int port)
{
    assert(NULL == webnet_task);
    webnet_port = port;
}

int webnet_get_port(void)
{
    return webnet_port;
}

void webnet_set_root(const char* webroot_path)
{
    strncpy(webnet_root, webroot_path, sizeof(webnet_root) - 1);
    webnet_root[sizeof(webnet_root) - 1] = '\0';
}

const char* webnet_get_root(void)
{
    return webnet_root;
}

/**
 * close webnet thread
 */
void wm_close_webnet_thread(void)
{
    if (NULL == webnet_task)
    {
        wm_log_debug("webnet task is null");
    }
    else
    {
        close_flag = 1;
        wm_os_internal_time_delay_ms(CLOSE_TIMEOUT_MS + 5);
    }
}

/**
 * webnet thread entry
 */
static void webnet_thread(void *parameter)
{
    int listenfd = -1;
    int keepalive = (int)parameter;
    fd_set readset, tempfds;
    fd_set writeset, tempwrtfds;
    int sock_fd, maxfdp1;
    struct sockaddr_in webnet_saddr;
    struct timeval rcv_to = {0, RECEIVE_TIMEOUT};
    struct timeval closetime = {0, CLOSE_TIMEOUT_MS * 1000};
    /* First acquire our socket for listening for connections */
    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenfd < 0)
    {
        wm_log_error("Create socket failed.");
        goto __exit;
    }
    wm_log_debug("Create socket success.");
    memset(&webnet_saddr, 0, sizeof(webnet_saddr));
    webnet_saddr.sin_family = AF_INET;
    webnet_saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    webnet_saddr.sin_port = htons(webnet_port); /* webnet server port */

    /* Set receive timeout for accept() */
    if (setsockopt(listenfd, SOL_SOCKET, SO_RCVTIMEO, (void*)&rcv_to, sizeof(rcv_to)) == -1)
    {
        wm_log_error("Set SO_RCVTIMEO failed, errno=%d", errno);
        goto __exit;
    }

    if (keepalive != 0)
    {
        int keepAlive = 1;
        int keepIdle = keepalive;
        int keepInterval = 1;
        int keepCount = 3;

        if (setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive)) < 0)
        {
            wm_log_error("set opt SO_KEEPALIVE error %s", strerror(errno));
            goto __exit;
        }


        if (setsockopt(listenfd, IPPROTO_TCP, TCP_KEEPIDLE, (void *)&keepIdle, sizeof(keepIdle)) < 0)
        {
            wm_log_error("set opt error %s", strerror(errno));
            goto __exit;
        }

        if (setsockopt(listenfd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval)) < 0)
        {
            wm_log_error("set opt error %s", strerror(errno));
            goto __exit;
        }

        if (setsockopt(listenfd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)) < 0)
        {
            wm_log_error("set opt error %s", strerror(errno));
            goto __exit;
        }
    }

    if (bind(listenfd, (struct sockaddr *) &webnet_saddr, sizeof(webnet_saddr)) == -1)
    {
        wm_log_error("Bind socket failed, errno=%d", errno);
        goto __exit;
    }

    /* Put socket into listening mode */
    if (listen(listenfd, CONFIG_WEBNET_CONN_MAX) == -1)
    {
        wm_log_error("Socket listen(%d) failed.", CONFIG_WEBNET_CONN_MAX);
        goto __exit;
    }

    /* initialize module (no session at present) */
    wm_log_debug("webnet_module_handle_event %s", "WEBNET_EVENT_INIT");
    webnet_module_handle_event(NULL, WEBNET_EVENT_INIT);

    /* Wait forever for network input: This could be connections or data */
    while (!close_flag)
    {
        /* Determine what sockets need to be in readset */
        FD_ZERO(&readset);
        FD_ZERO(&writeset);
        FD_SET(listenfd, &readset);

        /* set fds in each sessions */
        maxfdp1 = webnet_sessions_set_fds(&readset, &writeset);
        if (maxfdp1 < listenfd + 1)
        {
            maxfdp1 = listenfd + 1;
        }

        /* use temporary fd set in select */
        tempfds = readset;
        tempwrtfds = writeset;
        closetime.tv_usec = CLOSE_TIMEOUT_MS * 1000;
        /* Wait for data or a new connection */
        sock_fd = select(maxfdp1, &tempfds, &tempwrtfds, 0, &closetime);
        if (sock_fd == 0)
        {
            continue;
        }

        /* At least one descriptor is ready */
        if (FD_ISSET(listenfd, &tempfds))
        {
            struct webnet_session* accept_session;
            /* We have a new connection request */
            accept_session = webnet_session_create(listenfd);
            if (accept_session == NULL)
            {
                /* create session failed, just accept and then close */
                int sock;
                struct sockaddr cliaddr;
                socklen_t clilen;

                clilen = sizeof(struct sockaddr_in);
                sock = accept(listenfd, &cliaddr, &clilen);
                if (sock >= 0)
                {
                    closesocket(sock);
                }
            }
            else
            {
                /* add read fdset */
                FD_SET(accept_session->socket, &readset);
            }
        }

        webnet_sessions_handle_fds(&tempfds, &writeset);
    }

__exit:
    if (listenfd >= 0)
    {
        closesocket(listenfd);
    }

    wm_log_debug("close webnet task");

    close_flag = 0;
    webnet_task = NULL;
    wm_os_internal_task_del(NULL);
}

int webnet_init(int keep_alive)
{
    wm_os_status_t err;

    if (webnet_task != NULL)
    {
        wm_log_info("webnet is already initialized.");
        return 0;
    }
    err = wm_os_internal_task_create(&webnet_task,
                    "webnet_thread",
                    webnet_thread,
                    (void *)keep_alive,
                    WM_TASKWEBNET_STACK,
                    WEBNET_PRIORITY,
                    0);

    if (WM_OS_STATUS_SUCCESS != err)
    {
        wm_log_error("webnet initialize failed");
        return -1;
    }

    return 0;
}
