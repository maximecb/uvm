#include <stdio.h>
#include <string.h>
#include <uvm/syscalls.h>
#include <uvm/utils.h>

u64 listen_sock;

void on_new_conn(u64 socket_id)
{
    puts("got new connection");

    char client_addr[128];
    u64 conn_sock = net_accept(socket_id, client_addr, sizeof(128), on_incoming_data);
}

void on_incoming_data(u64 socket_id, u64 num_bytes)
{
    puts("got incoming data");

    char read_buf[1024];
    memset(read_buf, 0, 1024);
    net_read(socket_id, read_buf, 1024 - 1);
    puts(read_buf);

    char* response = "Hello!\n";
    net_write(socket_id, response, strlen(response));
}

void main()
{
    puts("starting TCP server");
    listen_sock = net_listen("127.0.0.1:9000", on_new_conn);

    enable_event_loop();
}
