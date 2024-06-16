#ifndef _WOLFUV_H_
#define _WOLFUV_H_

#include <uv.h>
#include <wolfssl/options.h>

struct wuv_tcp {
        uv_loop_t *loop;
        uv_tcp_t tcp;
        uv_poll_t handle;
        uv_buf_t buffer;
        uv_os_fd_t fd;

        /* libuv callbacks */
        uv_alloc_cb *alloc_cb;
        uv_read_cb *read_cb;
};

int     wuv_init(const char *cert_path, const char *priv_path);
void    wuv_deinit();
int     wuv_listen(struct wuv_tcp*, int, uv_connection_cb);
int     wuv_accept(struct uv_stream_t*, struct wuv_tcp*);
int     wuv_close(struct wuv_tcp*, uv_close_cb);

int     wuv_tcp_init(uv_loop_t*, struct wuv_tcp*);
int     wuv_tcp_bind(struct wuv_tcp*, const struct sockaddr*. unsigned int);

#endif
