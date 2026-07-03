//
// A minimal TCP echo server, reachable with `telnet 127.0.0.1 9000`.
//
// This shows the blocking UVM networking API: net_accept blocks until a client
// connects, and net_read blocks until data arrives. To serve several clients
// at once, the server spawns one thread per connection instead of relying on
// callbacks. Fallible calls report errors with negative return values
// (NET_ERROR, NET_TIMEOUT); see <uvm/syscalls.h>.
//

#include <stdio.h>
#include <string.h>
#include <uvm/syscalls.h>

// Handle a single client connection.
//
// One instance of this function runs per client, each on its own thread, so
// multiple clients can be served concurrently. The server echoes a reply for
// each message and closes the connection when the client sends "exit".
u64 handle_conn(u64 sock)
{
    char read_buf[1024];

    for (;;)
    {
        // net_read blocks until the client sends data or disconnects
        i64 num_bytes = net_read(sock, read_buf, sizeof(read_buf) - 1);

        // NET_EOF (0) means the client hung up; a negative value is an error
        if (num_bytes <= 0)
        {
            break;
        }

        // NUL-terminate the received bytes so we can treat them as a string
        read_buf[num_bytes] = 0;

        // Ignore telnet control sequences (bytes with the high bit set)
        if (read_buf[0] & 0x80)
        {
            continue;
        }

        printf("received: %s", read_buf);

        if (strncmp(read_buf, "exit", 4) == 0)
        {
            char* bye = "Goodbye!\n";
            net_write(sock, bye, strlen(bye));
            break;
        }

        // Echo a reply back to the client
        char* reply = "Hello!\n";
        net_write(sock, reply, strlen(reply));
    }

    net_close(sock);
    puts("connection closed");
    return 0;
}

int main()
{
    puts("starting TCP server on 127.0.0.1:9000");

    i64 listen_sock = net_listen("127.0.0.1:9000");
    if (listen_sock == NET_ERROR)
    {
        puts("failed to open listening socket");
        return 1;
    }

    // Accept connections forever, handing each one off to its own thread
    for (;;)
    {
        char client_addr[128];
        i64 conn_sock = net_accept(listen_sock, client_addr, sizeof(client_addr));

        // net_accept returns NET_ERROR if the listening socket is closed
        if (conn_sock == NET_ERROR)
        {
            break;
        }

        printf("new connection from %s\n", client_addr);
        thread_spawn(handle_conn, conn_sock);
    }

    return 0;
}
