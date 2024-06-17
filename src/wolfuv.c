#include <stdio.h>
#include <limits.h>

#include <uv.h>
#include <wolfuv.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfio.h>

static WOLFSSL_CTX *ctx = NULL;

static void     wolfuv_on_connection(uv_stream_t*, int);
static void     wolfuv_on_poll(uv_poll_t*, int, int);

int
wuv_init(const char *cert_path, const char *key_path)
{
        int ret;

        wolfSSL_Init();
        ctx = wolfSSL_CTX_new(wolfTLSv1_3_server_method());
    
        if ((ret = wolfSSL_CTX_use_certificate_file(ctx, cert_path,
                WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
                fprintf(stderr,
                    "ERROR: failed to load %s, please check the file.\n",
                    cert_path);
                return 1;
        }

        if (wolfSSL_CTX_use_PrivateKey_file(ctx, key_path, WOLFSSL_FILETYPE_PEM)
            != WOLFSSL_SUCCESS) {
                fprintf(stderr,
                    "ERROR: failed to load %s, please check the file.\n",
                    key_path);
                ret = -1;
                return 1;
        }

        return 0;
}

void
wuv_deinit()
{
        if (ctx != NULL)
                wolfSSL_CTX_free(ctx);
        wolfSSL_Cleanup();
}

int
wuv_tcp_init(uv_loop_t *loop, struct wuv_tcp *tcp)
{
        *tcp = (struct wuv_tcp) {0};
        tcp->loop = loop;
        tcp->state = WUV_ACCEPT;
        int ret = uv_tcp_init(loop, &tcp->tcp);
        tcp->tcp.data = tcp;
        return ret;
}

int
wuv_tcp_bind(struct wuv_tcp *tcp, const struct sockaddr *addr,
    unsigned int flags)
{
        return uv_tcp_bind(&tcp->tcp, addr, 0);
}

int
wuv_listen(struct wuv_tcp *tcp, int nconnections, wuv_connection_cb cb)
{
        tcp->connection_cb = cb;
        return uv_listen((uv_stream_t*) &tcp->tcp, nconnections,
            wolfuv_on_connection);
}

int
wuv_accept(struct wuv_tcp *server, struct wuv_tcp *client)
{
        int ret = uv_accept((uv_stream_t*) &server->tcp,
            (uv_stream_t*) &client->tcp);

        int ret2;

        if (ret != 0)
                return ret;

        uv_fileno(&client->tcp, &client->fd);

        if ((client->ssl = wolfSSL_new(ctx)) == NULL) {
                fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
                return 1;
        }

        if ((ret2 = wolfSSL_set_fd(client->ssl, client->fd))
            != WOLFSSL_SUCCESS) {
                fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
                return 1;
        }
        client->handle.data = client;

        uv_poll_init(client->loop, &client->handle, client->fd);
        uv_poll_start(&client->handle, UV_READABLE | UV_WRITABLE |
            UV_DISCONNECT, wolfuv_on_poll);

        return ret;
}

int
wuv_close(struct wuv_tcp *tcp, wuv_close_cb cb)
{
        tcp->close_cb = cb;
        tcp->state = WUV_DISCONNECT;
        return 0;
}

int
wuv_read_start(struct wuv_tcp *tcp, wuv_alloc_cb alloc_cb, wuv_read_cb read_cb)
{
        tcp->alloc_cb = alloc_cb;
        tcp->read_cb = read_cb;
        tcp->state = WUV_READ;
        return 0;
}

int
wuv_read_stop(struct wuv_tcp *tcp)
{
        tcp->state = WUV_IDLE;
        return 0;
}

int
wuv_write(struct wuv_tcp *tcp, uv_buf_t *buf, wuv_write_cb cb)
{
        tcp->prev_state = tcp->state;
        tcp->state = WUV_WRITE;
        tcp->write_buffer = *buf;
        tcp->write_cb = cb; 
        return 0;
}

static void
wolfuv_on_connection(uv_stream_t *server, int status)
{
        struct wuv_tcp *tcp = server->data;
        tcp->connection_cb(tcp, status);
}

static void
wolfuv_on_poll(uv_poll_t *handle, int status, int events)
{
        int ret;
        struct wuv_tcp *tcp = handle->data;

        if (events & UV_DISCONNECT) {
                tcp->state = WUV_DISCONNECT;
        }

        switch (tcp->state) {
        case WUV_ACCEPT:
                if (events & UV_READABLE && events & UV_WRITABLE) {
                        ret = wolfSSL_accept(tcp->ssl);
                        tcp->err = wolfSSL_get_error(tcp->ssl, ret);
                }

                if (ret == WOLFSSL_SUCCESS) {
                        tcp->err = WOLFSSL_ERROR_NONE;
                        tcp->state = WUV_IDLE;
                }
                break;
        case WUV_READ:
                if (events & UV_READABLE && tcp->err == WOLFSSL_ERROR_NONE) {
                        tcp->alloc_cb(tcp, USHRT_MAX + 1, &tcp->buffer);
                        tcp->read = 0;
                        goto read;
                }

                if (events & UV_READABLE && tcp->err ==
                    WOLFSSL_ERROR_WANT_READ) {
read:
                        ret = wolfSSL_read(tcp->ssl, tcp->buffer.base,
                            tcp->buffer.len);
                        tcp->err = wolfSSL_get_error(tcp->ssl, ret);

                        if (ret > 0) {
                            tcp->read += ret;
                        }
                }

                if (tcp->err != WOLFSSL_ERROR_WANT_READ) {
                        tcp->err = WOLFSSL_ERROR_NONE;
                        tcp->read_cb(tcp, tcp->read, &tcp->buffer);
                        tcp->read = 0;
                }
                break;
        case WUV_WRITE:
                if (events & UV_WRITABLE && tcp->write_buffer.len > 0) {
                        ret = wolfSSL_write(tcp->ssl,
                            tcp->write_buffer.base, tcp->write_buffer.len);
                        tcp->err = wolfSSL_get_error(tcp->ssl, ret);

                        if (ret > 0) {
                            tcp->write_buffer.len -= ret;
                        }
                }

                if (tcp->err != WOLFSSL_ERROR_WANT_WRITE
                    || tcp->write_buffer.len <= 0) {
                        tcp->err = WOLFSSL_ERROR_NONE;
                        tcp->state = tcp->prev_state;
                }
                break;
        case WUV_DISCONNECT:
                if (events & UV_READABLE && events & UV_WRITABLE) {
                        ret = wolfSSL_shutdown(tcp->ssl);
                        tcp->err = wolfSSL_get_error(tcp->ssl, ret);
                }

                if (tcp->err != WOLFSSL_ERROR_WANT_WRITE
                    && tcp->err != WOLFSSL_ERROR_WANT_READ) {
                        tcp->err = WOLFSSL_ERROR_NONE;
                        tcp->state = WUV_IDLE;
                        uv_poll_stop(&tcp->handle);
                        wolfSSL_free(tcp->ssl);
                        uv_close((uv_handle_t*) &tcp->tcp, NULL);

                        if (tcp->close_cb != NULL)
                                tcp->close_cb(tcp);
                        tcp->read_cb(tcp, UV_EOF, &tcp->buffer);
                }
                break;
        case WUV_IDLE:
                break;
        }
}
