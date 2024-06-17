#ifndef _WOLFUV_H_
#define _WOLFUV_H_

#include <uv.h>
#include <wolfssl/options.h>

typedef void (*wuv_alloc_cb)(struct wuv_tcp*, size_t, uv_buf_t*);
typedef void (*wuv_read_cb)(struct wuv_tcp*, ssize_t, const uv_buf_t*);
typedef void (*wuv_connection_cb)(struct wuv_tcp*, int);
typedef void (*wuv_close_cb)(struct wuv_tcp*);

struct wuv_tcp {
        uv_loop_t *loop;
        uv_tcp_t tcp;
        uv_poll_t handle;
        uv_buf_t buffer;
        uv_os_fd_t fd;

        /* libuv callbacks */
        wuv_alloc_cb *alloc_cb;
        wuv_read_cb *read_cb;
};

int     wuv_init(const char *cert_path, const char *priv_path);
void    wuv_deinit();
int     wuv_listen(struct wuv_tcp*, int, wuv_connection_cb);
int     wuv_accept(struct wuv_tcp*, struct wuv_tcp*);
int     wuv_close(struct wuv_tcp*, wuv_close_cb);
int     wuv_read_start(struct wuv_tcp*, wuv_alloc_cb, wuv_read_cb);
int     wuv_read_stop(struct wuv_tcp*);
int     wuv_write(uv_write_t*, struct wuv_tcp*, uv_buf_t *, unsigned int,
            uv_write_cb);

int     wuv_tcp_init(uv_loop_t*, struct wuv_tcp*);
int     wuv_tcp_bind(struct wuv_tcp*, const struct sockaddr*. unsigned int);

#endif
