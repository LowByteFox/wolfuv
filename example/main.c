// wolfuv - simple library that adds TLS support into libuv using wolfSSL
// Copyright (C) 2024  LowByteFox
// 
// wolfuv is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <uv.h>
#include <wolfuv.h>

uv_loop_t *loop;


void alloc(uv_handle_t *handle, size_t suggested, uv_buf_t *buf) {
    buf->base = malloc(suggested);
    buf->len = suggested;
}

void echo_write(uv_buf_t *req, int status) {
    if (status) {
        fprintf(stderr, "Write error %s\n", uv_strerror(status));
    }
    free(req->base);
}

void on_close(struct wuv_tcp *client) {
    free(client);
}

void on_read(struct wuv_tcp *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
            wuv_close(client, NULL);
        } else {
            printf("Client disconnect!\n");
            free(buf->base);
        }
    } else if (nread > 0) {
        uv_buf_t wrbuf = uv_buf_init(buf->base, nread);
        wuv_write(client, &wrbuf, echo_write);
    }
}

void on_connection(struct wuv_tcp *server, int status) {
    if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        return;
    }

    struct wuv_tcp *client = malloc(sizeof(struct wuv_tcp));
    wuv_tcp_init(loop, client);

    if (wuv_accept(server, client) == 0) {
        wuv_read_start(client, alloc, on_read);
    } else {
        wuv_close(client, NULL);
    }
}

int main() {
    wuv_init("./server-cert.pem", "./server-key.pem");
    loop = uv_default_loop();

    struct sockaddr_in addr;
    struct wuv_tcp server = {0};
    wuv_tcp_init(loop, &server);

    uv_ip4_addr("0.0.0.0", 7000, &addr);

    wuv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
    wuv_listen(&server, 128, on_connection);

    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
