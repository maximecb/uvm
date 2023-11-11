#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <uvm/syscalls.h>
#include <uvm/utils.h>

#define MAX_NAME_LEN 32
#define MAX_MSG_LEN 2048
#define MAX_USERS 512
#define MAX_MESSAGES 4096

// User states
#define STATE_DEFAULT 0
#define STATE_PICK_NAME 1
#define STATE_WRITE_MSG 2
#define STATE_READ_MSGS 3

typedef struct
{
    // Associated TCP socket
    u64 socket_id;

    // User name
    char name[MAX_NAME_LEN];

    // Current state
    int state;

    // Current index
    int cur_index;

    // Buffer for message writing
    char msg_buf[MAX_MSG_LEN];

} user_t;

typedef struct
{
    u64 timestamp;

    // We copy the username because user_t structs are ephemeral
    char username[MAX_NAME_LEN];

    char text[MAX_MSG_LEN];
} message_t;

user_t users[MAX_USERS];
message_t messages[MAX_MESSAGES];

// Number of online users
size_t num_users = 0;

// Number of messages posted
size_t num_messages = 0;

// TCP socket to listen for new connections
u64 listen_sock;

// Process start time
u64 start_time;

// Find a user by socket id
user_t* find_user(u64 socket_id)
{
    for (int i = 0; i < MAX_USERS; ++i)
    {
        user_t* p_user = &users[i];

        if (p_user->socket_id == socket_id)
        {
            return p_user;
        }
    }

    return NULL;
}

// Allocate a new user object
user_t* alloc_user(u64 client_sock)
{
    if (num_users >= MAX_USERS)
        return NULL;

    // Try to find a free user struct
    user_t* p_user = find_user(0);

    if (!p_user)
        return NULL;

    memset(p_user, 0, sizeof(user_t));
    p_user->socket_id = client_sock;

    // Increment the number of users connected
    ++num_users;

    return p_user;
}

void free_user(user_t* p_user)
{
    memset(p_user, 0, sizeof(user_t));
    --num_users;
}

// Strip trailing whitespace from a string
void rstrip_ws(char* buf)
{
    int len = (int)strlen(buf);

    for (int i = len - 1; i > 0; --i)
    {
        if (isspace(buf[i]))
        {
            buf[i] = '\0';
        }
        else
        {
            break;
        }
    }
}

void post_message(user_t* p_user)
{
    // If we're at capacity, drop the first message, shift the messages back
    if (num_messages == MAX_MESSAGES)
    {
        for (int i = 0; i < num_messages - 1; ++i)
        {
            memcpy(&messages[i], &messages[i+1], sizeof(message_t));
        }

        --num_messages;
    }

    // Find a free message struct
    size_t msg_idx = num_messages;
    ++num_messages;
    assert(num_messages <= MAX_MESSAGES);
    message_t* p_message = &messages[msg_idx];

    p_message->timestamp = time_current_ms();

    memcpy(p_message->username, p_user->name, MAX_NAME_LEN);

    //p_user->msg_buf[MAX_MSG_LEN - 1] = '\0';
    memcpy(p_message->text, p_user->msg_buf, sizeof(p_user->msg_buf));
}

void write_str(u64 socket_id, char* str)
{
    net_write(socket_id, str, strlen(str));
}

void write_int(u64 socket_id, int val)
{
    char buf[32];
    itoa(val, buf, 10);
    write_str(socket_id, buf);
}

void view_cur_message(user_t* p_user)
{
    u64 socket_id = p_user->socket_id;

    write_str(socket_id, "\n");
    write_str(socket_id, "##################################################################\n");
    write_str(socket_id, "Reading message ");
    write_int(socket_id, (int)p_user->cur_index + 1);
    write_str(socket_id, "/");
    write_int(socket_id, (int)num_messages);
    write_str(socket_id, ". Enter (N) for next message, (H) for help.\n");
    write_str(socket_id, "##################################################################\n");
    write_str(socket_id, "\n");

    assert(p_user->cur_index < num_messages);
    message_t* p_message = &messages[p_user->cur_index];
    int n_seconds = (int)((time_current_ms() - p_message->timestamp) / 1000);

    write_str(socket_id, "@");
    write_str(socket_id, p_message->username);
    write_str(socket_id, " wrote ");
    write_int(socket_id, n_seconds);
    write_str(socket_id, " seconds ago:\n");
    write_str(socket_id, p_message->text);
    write_str(socket_id, "\n");
    write_str(socket_id, "\n");
}

void write_help(u64 socket_id)
{
    char* help_msg = (
        "\n"
        "Available commands:\n"
        "(R) Read messages\n"
        "(P) Post new message\n"
        "(H) Help\n"
        "(A) About\n"
        "(E) Exit\n"
        "\n"
    );

    write_str(socket_id, help_msg);
}

void write_post_help(u64 socket_id)
{
    write_str(socket_id, "\n");
    write_str(socket_id, "##############################################################\n");
    write_str(socket_id, "You may now write your message.\n");
    write_str(socket_id, "You can include newlines and paragraphs in your message.\n");
    write_str(socket_id, "Simply write Done and then press return to signal you are\n");
    write_str(socket_id, "done writing. The maximum message length is 4096 characters.\n");
    write_str(socket_id, "##############################################################\n");
    write_str(socket_id, "\n");
}

void on_new_conn(u64 listen_sock)
{
    puts("got new connection");

    char client_addr[128];
    u64 client_sock = net_accept(listen_sock, client_addr, sizeof(client_addr), on_incoming_data);

    printf("client address: %s\n", client_addr);

    user_t* p_user = alloc_user(client_sock);

    if (!p_user)
    {
        puts("Could not find free user struct");
        write_str(client_sock, "Too many users connected. Please come back later! :)\n");
        net_close(client_sock);
        return;
    }

    char* welcome_msg = (
        "\n"
        "##########################################################\n"
        "#\n"
        "# Welcome to the UVM Mini BBS!\n"
        "#\n"
        "# Visit the GitHub repository for more info:\n"
        "# https://github.com/maximecb/uvm\n"
        "#\n"
        "##########################################################\n"
    );
    write_str(client_sock, welcome_msg);

    // Print uptime
    u64 uptime = (time_current_ms() - start_time) / 1000;
    char uptime_str[32];
    itoa((int)uptime, uptime_str, 10);
    write_str(client_sock, "\nUptime is ");
    write_str(client_sock, uptime_str);
    write_str(client_sock, " seconds since the last restart or crash\n");

    // Print users online
    char num_users_str[32];
    itoa((int)num_users, num_users_str, 10);
    write_str(client_sock, num_users_str);
    write_str(client_sock, " user(s) currently connected\n");

    // Print messages posted
    char num_msgs_str[32];
    itoa((int)num_messages, num_msgs_str, 10);
    write_str(client_sock, num_msgs_str);
    write_str(client_sock, " message(s) stored in memory\n");

    write_str(client_sock, "\nEnter a command and then press the return key\n");
    write_help(client_sock);
}

void on_incoming_data(u64 socket_id, u64 num_bytes)
{
    printf("received %d bytes of incoming data\n", num_bytes);

    char read_buf[2048];
    memset(read_buf, 0, sizeof(read_buf));

    // If we receive a ton of bytes at once, this is likely
    // malicious, close the socket and ignore it.
    if (num_bytes > sizeof(read_buf) - 1)
    {
        net_close(socket_id);
        return;
    }

    net_read(socket_id, read_buf, sizeof(read_buf) - 1);

    // If the buffer contains control characters, ignore it
    for (int i = 0; i < sizeof(read_buf); ++i)
    {
        char ch = read_buf[i];
        if (ch == 0)
            break;
        if (!isprint(ch) && ch != '\r' && ch != '\n')
            return;
    }

    // Find the user associated with this socket
    user_t* p_user = find_user(socket_id);

    if (!p_user)
    {
        puts("could not find user for socket id");
        return;
    }

    puts(read_buf);

    if (p_user->state == STATE_PICK_NAME)
    {
        // Strip trailing whitespace
        rstrip_ws(read_buf);

        size_t len = strlen(read_buf);

        if (len == 0)
        {
            write_str(socket_id, "Username too short. Try again:\n");
            return;
        }

        if (len > MAX_NAME_LEN - 1)
        {
            write_str(socket_id, "Username too long. Try again:\n");
            return;
        }

        for (int i = 0; i < len; ++i)
        {
            if (!isalnum(read_buf[i]) && read_buf[i] != '_')
            {
                write_str(socket_id, "Username should only contain alphanumeric characters and underscores. Try again:\n");
                return;
            }
        }

        strncpy(p_user->name, read_buf, len);
        write_str(socket_id, "Welcome ");
        write_str(socket_id, p_user->name);
        write_str(socket_id, "!\n");

        p_user->state = STATE_WRITE_MSG;
        write_post_help(socket_id);

        return;
    }

    if (p_user->state == STATE_WRITE_MSG)
    {
        bool done = (
            strcmp(read_buf, "Done\n") == 0     ||
            strcmp(read_buf, "Done\r\n") == 0   ||
            strcmp(read_buf, "done\n") == 0     ||
            strcmp(read_buf, "done\r\n") == 0
        );

        if (done)
        {
            // Strip trailing whitespace
            rstrip_ws(p_user->msg_buf);

            // If this is an empty message, do nothing
            if (strlen(p_user->msg_buf) > 0)
            {
                post_message(p_user);

                write_str(socket_id, "\n");
                write_str(socket_id, "Message posted! :)\n");
                write_help(socket_id);
            }
            else
            {
                write_str(socket_id, "\n");
                write_str(socket_id, "Skipping empty message.\n");
                write_help(socket_id);
            }

            p_user->state = STATE_DEFAULT;
            memset(p_user->msg_buf, 0, sizeof(p_user->msg_buf));

            return;
        }

        size_t msg_len = strlen(p_user->msg_buf);
        size_t len = strlen(read_buf);

        size_t max_chars = MAX_MSG_LEN - 1 - msg_len;
        strncat(p_user->msg_buf, read_buf, max_chars);

        printf("strlen(p_user->msg_buf)=%d\n", strlen(p_user->msg_buf));

        return;
    }

    char ch = toupper(read_buf[0]);

    // About
    if (ch == 'A')
    {
        write_str(
            socket_id,
            "\n"
            "~~~ UVM Mini BBS ~~~\n"
            "An ephemeral BBS experiment to test out the UVM networking API,\n"
            "and for me to practice C/socket programming.\n"
            "The port number is 9001, because it's over nine thousands.\n"
            "Messages are stored in RAM only, so if the server crashes\n"
            "or restarts, everything is cleared.\n"
            "Visit https://github.com/maximecb/uvm for more! :)\n"
            "\n"
        );
        return;
    }

    // Exit/Quit
    if (ch == 'E' || ch == 'Q')
    {
        puts("Got exit command");
        char* response = "Goodbye!\n";
        net_write(socket_id, response, strlen(response));
        net_close(socket_id);
        free_user(socket_id);
        return;
    }

    // Post message
    if (ch == 'P')
    {
        if (strlen(p_user->name) == 0)
        {
            p_user->state = STATE_PICK_NAME;
            write_str(socket_id, "\n");
            write_str(socket_id, "Please choose a username and then press the return key:\n");
        }
        else
        {
            p_user->state = STATE_WRITE_MSG;
            write_post_help(socket_id);
        }

        return;
    }

    // Read message
    if (ch == 'R')
    {
        if (num_messages == 0)
        {
            write_str(socket_id, "\n");
            write_str(socket_id, "There are no messages to read. Enter (P) to write one!\n");
            write_str(socket_id, "\n");
            return;
        }

        p_user->state = STATE_READ_MSGS;
        p_user->cur_index = (int)num_messages - 1;
        view_cur_message(p_user);
        return;
    }

    // Next message
    if (ch == 'N' && p_user->state == STATE_READ_MSGS)
    {
        if (p_user->cur_index > 0)
            p_user->cur_index = p_user->cur_index - 1;
        else
            p_user->cur_index = (int)num_messages - 1;

        view_cur_message(p_user);
        return;
    }

    //write_str(socket_id, "\nUnknown command\n\n");
    write_help(socket_id);
}

void main()
{
    start_time = time_current_ms();

    puts("Starting TCP server");
    listen_sock = net_listen("0.0.0.0:9001", on_new_conn);

    enable_event_loop();
}
