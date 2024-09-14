#include "rfc2217_server.h"
#include "pthread.h"
#include <stdbool.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "esp_log.h"
#include "esp_pthread.h"

static const char *TAG = "rfc2217_server";
// telnet
#define T_SE 0xf0U
#define T_NOP 0xf1U
#define T_DM 0xf2U
#define T_BRK 0xf3U
#define T_IP 0xf4U
#define T_AO 0xf5U
#define T_AYT 0xf6U
#define T_EC 0xf7U
#define T_EL 0xf8U
#define T_GA 0xf9U
#define T_SB 0xfaU
#define T_WILL 0xfbU
#define T_WONT 0xfcU
#define T_DO 0xfdU
#define T_DONT 0xfeU
#define T_IAC 0xffU

// telnet options
#define T_BINARY 0x00U
#define T_ECHO 0x01U
#define T_SGA 0x03U

// RFC2217
#define T_COM_PORT_OPTION 0x2cU

// Client to server
#define T_SET_BAUDRATE 0x01U
#define T_SET_DATASIZE 0x02U
#define T_SET_PARITY 0x03U
#define T_SET_STOPSIZE 0x04U
#define T_SET_CONTROL 0x05U
#define T_NOTIFY_LINESTATE 0x06U
#define T_NOTIFY_MODEMSTATE 0x07U
#define T_FLOWCONTROL_SUSPEND 0x08U
#define T_FLOWCONTROL_RESUME 0x09U
#define T_SET_LINESTATE_MASK 0x0aU
#define T_SET_MODEMSTATE_MASK 0x0bU
#define T_PURGE_DATA 0x0cU

// Server to client
#define T_SERVER_SET_BAUDRATE 0x65U
#define T_SERVER_SET_DATASIZE 0x66U
#define T_SERVER_SET_PARITY 0x67U
#define T_SERVER_SET_STOPSIZE 0x68U
#define T_SERVER_SET_CONTROL 0x69U
#define T_SERVER_NOTIFY_LINESTATE 0x6aU
#define T_SERVER_NOTIFY_MODEMSTATE 0x6bU
#define T_SERVER_FLOWCONTROL_SUSPEND 0x6cU
#define T_SERVER_FLOWCONTROL_RESUME 0x6dU
#define T_SERVER_SET_LINESTATE_MASK 0x6eU
#define T_SERVER_SET_MODEMSTATE_MASK 0x6fU
#define T_SERVER_PURGE_DATA 0x70U

typedef enum { T_NORMAL, T_GOT_IAC, T_NEGOTIATE } telnet_mode_t;


typedef enum {
    T_REQUESTED,
    T_ACTIVE,
    T_INACTIVE,
    T_REALLY_INACTIVE
} telnet_option_state_t;

typedef void (*activation_callback_t)(rfc2217_server_t, bool);
typedef void (*send_option_callback_t)(rfc2217_server_t, uint8_t, uint8_t);
typedef struct {
    uint8_t option;
    const char *name;
    telnet_option_state_t initial_state;
    uint8_t send_yes;
    uint8_t send_no;
    uint8_t ack_yes;
    uint8_t ack_no;
    activation_callback_t cb;
    send_option_callback_t send_option_cb;
} telnet_option_def_t;

typedef struct {
    const telnet_option_def_t *def;
    void *ctx;
    bool active;
    telnet_option_state_t state;
} telnet_option_t;

struct rfc2217_server_s {
    rfc2217_server_config_t config;
    uint8_t tcp_rx_buffer[128];
    telnet_mode_t telnet_mode;
    int client_socket;
    pthread_t server_thread;
    volatile bool server_thread_shutdown;
    pthread_t tcp_receive_thread;
    volatile bool tcp_receive_thread_shutdown;
    bool client_is_rfc2217;
    pthread_mutex_t tcp_send_mutex;
    uint8_t suboption[16];
    size_t suboption_size;
    telnet_option_t *telnet_options;
    size_t telnet_options_count;
    bool collecting_suboption;
    uint8_t telnet_command;
};

static void *server_thread_fn(void *ctx /* rfc2217_server_t server */);
static void *tcp_receive_thread_fn(void *ctx /* rfc2217_server_t server */);

static void process_received_over_tcp(rfc2217_server_t server, const uint8_t *buf, size_t size);
static void tcp_send(rfc2217_server_t server, const void *buf, size_t size);
static void process_subnegotiation(rfc2217_server_t server);
static void process_telnet_command(rfc2217_server_t server, uint8_t c);
static void telnet_negotiate_option(rfc2217_server_t server, uint8_t command, uint8_t option);
static void telnet_send_option(rfc2217_server_t server, uint8_t action, uint8_t option);
static void on_client_ok(rfc2217_server_t server, bool ok);
static void rfc2217_send_subnegotiation(rfc2217_server_t server, uint8_t command, const uint8_t *data, size_t size);
static int telnet_options_init(rfc2217_server_t server);
static void telnet_options_destroy(rfc2217_server_t server);


void telnet_option_process_incoming(telnet_option_t *option, uint8_t command)
{
    ESP_LOGD(TAG, "Option %s (state=%d) received command 0x%x", option->def->name, option->state, command);
    if (command == option->def->ack_yes) {
        if (option->state == T_REQUESTED) {
            option->state = T_ACTIVE;
            option->active = true;
            ESP_LOGD(TAG, "Option %s activated", option->def->name);
            if (option->def->cb) {
                option->def->cb(option->ctx, true);
            }
        } else if (option->state == T_ACTIVE) {
            // Do nothing
        } else if (option->state == T_INACTIVE) {
            option->state = T_ACTIVE;
            if (option->def->send_option_cb) {
                option->def->send_option_cb(option->ctx, option->def->send_yes, option->def->option);
            }
            option->active = true;
            if (option->def->cb) {
                option->def->cb(option->ctx, true);
            }
        } else if (option->state == T_REALLY_INACTIVE) {
            if (option->def->send_option_cb) {
                option->def->send_option_cb(option->ctx, option->def->send_no, option->def->option);
            }
        }
    } else if (command == option->def->ack_no) {
        if (option->state == T_REQUESTED) {
            option->state = T_INACTIVE;
            option->active = false;
        } else if (option->state == T_ACTIVE) {
            option->state = T_INACTIVE;
            if (option->def->send_option_cb) {
                option->def->send_option_cb(option->ctx, option->def->send_no, option->def->option);
            }
            option->active = false;
        } else if (option->state == T_INACTIVE) {
            // Do nothing
        } else if (option->state == T_REALLY_INACTIVE) {
            // Do nothing
        }
    }
    ESP_LOGD(TAG, "Option %s state: %d active: %d", option->def->name, option->state, option->active);
}

static int telnet_options_init(rfc2217_server_t server)
{
    static const telnet_option_def_t option_defs[] = {
        {T_ECHO, "ECHO", T_REQUESTED, T_WILL, T_WONT, T_DO, T_DONT, NULL, NULL},
        {T_SGA, "we-SGA", T_REQUESTED, T_WILL, T_WONT, T_DO, T_DONT, NULL, NULL},
        {T_SGA, "they-SGA", T_INACTIVE, T_DO, T_DONT, T_WILL, T_WONT, NULL, NULL},
        {T_BINARY, "we-BINARY", T_INACTIVE, T_WILL, T_WONT, T_DO, T_DONT, NULL, NULL},
        {T_BINARY, "they-BINARY", T_REQUESTED, T_DO, T_DONT, T_WILL, T_WONT, NULL, NULL},
        {T_COM_PORT_OPTION, "we-RFC2217", T_REQUESTED, T_WILL, T_WONT, T_DO, T_DONT, on_client_ok, telnet_send_option},
        {T_COM_PORT_OPTION, "they-RFC2217", T_INACTIVE, T_DO, T_DONT, T_WILL, T_WONT, on_client_ok, telnet_send_option}
    };

    server->telnet_options_count = sizeof(option_defs) / sizeof(option_defs[0]);

    telnet_option_t *options = calloc(server->telnet_options_count, sizeof(telnet_option_t));
    if (!options) {
        ESP_LOGE(TAG, "Failed to allocate memory for telnet options");
        return -1;
    }

    for (size_t i = 0; i < server->telnet_options_count; i++) {
        options[i].def = &option_defs[i];
        options[i].ctx = server;
        options[i].state = option_defs[i].initial_state;
        options[i].active = false;
    }

    server->telnet_options = options;
    return 0;
}

static void telnet_options_destroy(rfc2217_server_t server)
{
    free(server->telnet_options);
}

int rfc2217_server_create(const rfc2217_server_config_t *config, rfc2217_server_t *out_server)
{
    rfc2217_server_t server = calloc(1, sizeof(struct rfc2217_server_s));
    if (!server) {
        return -1;
    }

    server->config = *config;
    server->telnet_mode = T_NORMAL;
    pthread_mutex_init(&server->tcp_send_mutex, NULL);
    *out_server = server;
    return 0;
}

int rfc2217_server_start(rfc2217_server_t server)
{


    int res = pthread_create(&server->server_thread, NULL, server_thread_fn, server);
    if (res != 0) {
        ESP_LOGE(TAG, "Failed to create server thread: %d", res);
        return -1;
    }

    return 0;
}

int rfc2217_server_stop(rfc2217_server_t server)
{
    // check if the server thread is running
    if (server->server_thread == 0) {
        ESP_LOGE(TAG, "Server thread is not running");
        return -1;
    }
    // stop the tcp receive thread
    server->tcp_receive_thread_shutdown = true;
    pthread_join(server->tcp_receive_thread, NULL);
    server->tcp_receive_thread = 0;
    // stop the server thread
    server->server_thread_shutdown = true;
    pthread_join(server->server_thread, NULL);
    server->server_thread = 0;
    return 0;
}

void rfc2217_server_destroy(rfc2217_server_t server)
{
    free(server);
}

void *server_thread_fn(void *ctx /* rfc2217_server_t server */)
{
    rfc2217_server_t server = (rfc2217_server_t)ctx;
    struct sockaddr_storage dest_addr = {};
    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(server->config.port);

    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d (%s)", errno, strerror(errno));
        return NULL;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(*dest_addr_ip4));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d (%s)", errno, strerror(errno));
        goto CLEAN_UP;
    }
    ESP_LOGD(TAG, "Socket bound, port %d", server->config.port);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d (%s)", errno, strerror(errno));
        goto CLEAN_UP;
    }

    while (true) {
        ESP_LOGD(TAG, "Socket listening");

        struct sockaddr_storage source_addr = {}; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        server->client_socket = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (server->client_socket < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d (%s)", errno, strerror(errno));
            break;
        }

        server->tcp_receive_thread_shutdown = false;
        pthread_create(&server->tcp_receive_thread, NULL, tcp_receive_thread_fn, server);

        pthread_join(server->tcp_receive_thread, NULL);

        shutdown(server->client_socket, 0);
        close(server->client_socket);
    }
    ESP_LOGD(TAG, "Server thread shutting down");

CLEAN_UP:
    close(listen_sock);

    pthread_exit(NULL);
}


static void *tcp_receive_thread_fn(void *ctx /* rfc2217_server_t server */)
{
    rfc2217_server_t server = (rfc2217_server_t)ctx;
    ESP_LOGI(TAG, "TCP receive thread started, socket: %d", server->client_socket);
    ssize_t len;

    telnet_options_init(server);
    server->client_is_rfc2217 = false;
    server->collecting_suboption = false;
    server->suboption_size = 0;
    server->telnet_mode = T_NORMAL;

    do {
        len = recv(server->client_socket, server->tcp_rx_buffer, sizeof(server->tcp_rx_buffer) - 1, 0);


        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d (%s)", errno, strerror(errno));
        } else if (len == 0) {
            ESP_LOGI(TAG, "Connection closed");
        } else {
            server->tcp_rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string

            ESP_LOGD(TAG, "Received %d bytes:", (int) len);
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, server->tcp_rx_buffer, len, ESP_LOG_DEBUG);
            process_received_over_tcp(server, server->tcp_rx_buffer, len);
        }
    } while (len > 0);
    if (server->config.on_client_disconnected) {
        server->config.on_client_disconnected(server->config.ctx);
    }
    telnet_options_destroy(server);
    ESP_LOGI(TAG, "TCP receive thread done");
    pthread_exit(NULL);
}

static void tcp_send(rfc2217_server_t server, const void *buf, size_t size)
{
    pthread_mutex_lock(&server->tcp_send_mutex);
    const uint8_t *cbuf = (const uint8_t *)buf;
    size_t to_write = size;
    while (to_write > 0) {
        ssize_t written = send(server->client_socket, cbuf, to_write, 0);
        if (written < 0) {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            pthread_mutex_unlock(&server->tcp_send_mutex);
            return;
        }
        to_write -= written;
        cbuf += written;
    }
    pthread_mutex_unlock(&server->tcp_send_mutex);
}

static void process_received_over_tcp(rfc2217_server_t server, const uint8_t *buf, size_t size)
{
    // fast path: if we are not in telnet mode and there is no IAC, just pass the data on to the callback
    if (server->telnet_mode == T_NORMAL && memchr(buf, T_IAC, size) == NULL) {
        if (server->config.on_data_received) {
            server->config.on_data_received(server->config.ctx, buf, size);
        }
        return;
    }

    // otherwise, process the data byte by byte
    const uint8_t *end = buf + size;
    for (; buf < end; ++buf) {
        uint8_t c = *buf;
        switch (server->telnet_mode) {
        case T_NORMAL: {
            if (c == T_IAC) {
                server->telnet_mode = T_GOT_IAC;
            } else if (server->collecting_suboption) {
                if (server->suboption_size < sizeof(server->suboption)) {
                    server->suboption[server->suboption_size++] = c;
                } else {
                    ESP_LOGE(TAG, "Suboption buffer overflow");
                    server->collecting_suboption = false;
                    server->suboption_size = 0;
                }
            } else {
                if (server->config.on_data_received) {
                    server->config.on_data_received(server->config.ctx, &c, 1);
                }
            }
            break;
        }
        case T_GOT_IAC: {
            if (c == T_IAC) {
                if (server->collecting_suboption) {
                    if (server->suboption_size < sizeof(server->suboption)) {
                        server->suboption[server->suboption_size++] = c;
                    } else {
                        ESP_LOGE(TAG, "Suboption buffer overflow");
                        server->collecting_suboption = false;
                        server->suboption_size = 0;
                    }
                } else {
                    if (server->config.on_data_received) {
                        server->config.on_data_received(server->config.ctx, &c, 1);
                    }
                }
                server->telnet_mode = T_NORMAL;
            } else if (c == T_SB) {
                server->suboption_size = 0;
                server->collecting_suboption = true;
                server->telnet_mode = T_NORMAL;
            } else if (c == T_SE) {
                process_subnegotiation(server);
                server->suboption_size = 0;
                server->collecting_suboption = false;
                server->telnet_mode = T_NORMAL;
            } else if (c == T_WILL || c == T_WONT || c == T_DO || c == T_DONT) {
                server->telnet_command = c;
                server->telnet_mode = T_NEGOTIATE;
            } else {
                process_telnet_command(server, c);
                server->telnet_mode = T_NORMAL;
            }
            break;
        }
        case T_NEGOTIATE: {
            telnet_negotiate_option(server, server->telnet_command, c);
            server->telnet_mode = T_NORMAL;
            break;
        }
        }
    }
}


int rfc2217_server_send_data(rfc2217_server_t server, const uint8_t *data, size_t len)
{
    if (server->client_socket < 0) {
        ESP_LOGE(TAG, "Client socket is not connected");
        return -1;
    }
    if (server->tcp_receive_thread == 0) {
        ESP_LOGE(TAG, "TCP receive thread is not running");
        return -1;
    }
    tcp_send(server, data, len);
    return 0;
}


static void process_telnet_command(rfc2217_server_t server, uint8_t c)
{
    ESP_LOGD(TAG, "Ignoring telnet command: %d", c);
}

static void telnet_negotiate_option(rfc2217_server_t server, uint8_t command, uint8_t option)
{
    ESP_LOGD(TAG, "Telnet negotiate option: 0x%x 0x%x", command, option);

    bool known = false;
    for (size_t i = 0; i < server->telnet_options_count; i++) {
        ESP_LOGD(TAG, "Trying option %s (0x%x)", server->telnet_options[i].def->name, server->telnet_options[i].def->option);
        if (server->telnet_options[i].def->option == option) {
            telnet_option_process_incoming(&server->telnet_options[i], command);
            known = true;
        }
    }
    if (!known) {
        ESP_LOGD(TAG, "Unknown option: 0x%x", option);
        if (command == T_WILL) {
            telnet_send_option(server, T_DONT, option);
        }
        if (command == T_DO) {
            telnet_send_option(server, T_WONT, option);
        }
    }
}

void telnet_send_option(rfc2217_server_t server, uint8_t action, uint8_t option)
{
    uint8_t buf[3] = {T_IAC, action, option};
    tcp_send(server, buf, sizeof(buf));
}

void on_client_ok(rfc2217_server_t server, bool ok)
{
    if (!server->client_is_rfc2217) {
        ESP_LOGD(TAG, "Client is RFC2217");
        server->client_is_rfc2217 = true;
        if (server->config.on_client_connected) {
            server->config.on_client_connected(server->config.ctx);
        }
    }
}


static void process_subnegotiation(rfc2217_server_t server)
{
    ESP_LOGD(TAG, "Processing subnegotiation");
    if (server->suboption[0] != T_COM_PORT_OPTION) {
        ESP_LOGD(TAG, "Unknown subnegotiation: %x", server->suboption[0]);
        return;
    }

    uint8_t subnegotiation = server->suboption[1];
    if (subnegotiation == T_SET_BAUDRATE) {
        uint32_t baudrate = (server->suboption[2] << 24) | (server->suboption[3] << 16) | (server->suboption[4] << 8) | server->suboption[5];
        uint32_t new_baudrate = baudrate;
        if (server->config.on_baudrate) {
            new_baudrate = server->config.on_baudrate(server->config.ctx, baudrate);
        }
        ESP_LOGD(TAG, "Set baudrate: requested %d, accepted %d", baudrate, baudrate);
        uint8_t data[4] = {new_baudrate >> 24, new_baudrate >> 16, new_baudrate >> 8, new_baudrate};
        rfc2217_send_subnegotiation(server, T_SERVER_SET_BAUDRATE, data, 4);
    } else if (subnegotiation == T_SET_DATASIZE) {
        uint8_t datasize = server->suboption[2];
        ESP_LOGD(TAG, "Set datasize: %d - not supported, accepting", datasize);
        rfc2217_send_subnegotiation(server, T_SERVER_SET_DATASIZE, &server->suboption[2], 1);
    } else if (subnegotiation == T_SET_PARITY) {
        uint8_t parity = server->suboption[2];
        ESP_LOGD(TAG, "Set parity: %d - not supported, accepting", parity);
        rfc2217_send_subnegotiation(server, T_SERVER_SET_PARITY, &server->suboption[2], 1);
    } else if (subnegotiation == T_SET_STOPSIZE) {
        uint8_t stopsize = server->suboption[2];
        ESP_LOGD(TAG, "Set stopsize: %d - not supported, accepting", stopsize);
        rfc2217_send_subnegotiation(server, T_SERVER_SET_STOPSIZE, &server->suboption[2], 1);
    } else if (subnegotiation == T_SET_CONTROL) {
        uint8_t control_byte = server->suboption[2];
        rfc2217_control_t control = (rfc2217_control_t)control_byte;
        rfc2217_control_t new_control = control;
        if (server->config.on_control) {
            new_control = server->config.on_control(server->config.ctx, control);
        }
        ESP_LOGD(TAG, "Set control: requested %d, accepted %d", control, new_control);
        uint8_t data[1] = {new_control};
        rfc2217_send_subnegotiation(server, T_SERVER_SET_CONTROL, data, 1);
    } else if (subnegotiation == T_NOTIFY_LINESTATE) {
        uint8_t linestate = server->suboption[2];
        ESP_LOGD(TAG, "Notify linestate: %d - not supported", linestate);
        rfc2217_send_subnegotiation(server, T_SERVER_NOTIFY_LINESTATE, &server->suboption[2], 1);
    } else if (subnegotiation == T_NOTIFY_MODEMSTATE) {
        uint8_t modemstate = server->suboption[2];
        ESP_LOGD(TAG, "Notify modemstate: %d - not supported", modemstate);
        rfc2217_send_subnegotiation(server, T_SERVER_NOTIFY_MODEMSTATE, &server->suboption[2], 1);
    } else if (subnegotiation == T_PURGE_DATA) {
        uint8_t purge = server->suboption[2];
        rfc2217_send_subnegotiation(server, T_SERVER_PURGE_DATA, &server->suboption[2], 1);
        rfc2217_purge_t purge_type = (rfc2217_purge_t)purge;
        rfc2217_purge_t purge_result = purge_type;
        if (server->config.on_purge) {
            server->config.on_purge(server->config.ctx, purge_type);
        }
        ESP_LOGD(TAG, "Purge data: requested %d, accepted %d", purge_type, purge_result);
        uint8_t data[1] = {purge_result};
        rfc2217_send_subnegotiation(server, T_SERVER_PURGE_DATA, data, 1);
    } else {
        ESP_LOGD(TAG, "Unknown subnegotiation: %x", subnegotiation);
    }
}

void rfc2217_send_subnegotiation(rfc2217_server_t server, uint8_t command, const uint8_t *data, size_t size)
{
    uint8_t buf[256]; // Adjust size as needed
    size_t buf_index = 0;

    buf[buf_index++] = T_IAC;
    buf[buf_index++] = T_SB;
    buf[buf_index++] = T_COM_PORT_OPTION;
    buf[buf_index++] = command;

    // escape IAC
    for (size_t i = 0; i < size; i++) {
        if (data[i] == T_IAC) {
            buf[buf_index++] = T_IAC;
        }
        buf[buf_index++] = data[i];
    }

    buf[buf_index++] = T_IAC;
    buf[buf_index++] = T_SE;

    tcp_send(server, buf, buf_index);
}
