#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RFC2217 server instance handle
 */
typedef struct rfc2217_server_s *rfc2217_server_t;

/**
 * @brief RFC2217 control signal definitions
 * FIXME: split this into separate enums and callbacks
 */
typedef enum {
    RFC2217_CONTROL_SET_NO_FLOW_CONTROL = 1,  //!< Set no flow control
    RFC2217_CONTROL_SET_XON_XOFF_FLOW_CONTROL = 2,  //!< Set XON/XOFF flow control
    RFC2217_CONTROL_SET_HARDWARE_FLOW_CONTROL = 3,  //!< Set hardware flow control
    RFC2217_CONTROL_SET_BREAK = 5,  //!< Set break signal
    RFC2217_CONTROL_CLEAR_BREAK = 6,    //!< Clear break signal
    RFC2217_CONTROL_SET_DTR = 8,    //!< Set DTR signal
    RFC2217_CONTROL_CLEAR_DTR = 9,  //!< Clear DTR signal
    RFC2217_CONTROL_SET_RTS = 11,   //!< Set RTS signal
    RFC2217_CONTROL_CLEAR_RTS = 12  //!< Clear RTS signal
} rfc2217_control_t;

/**
 * @brief RFC2217 purge request definitions
 */
typedef enum {
    RFC2217_PURGE_RECEIVE = 0,      //!< Request to purge receive buffer
    RFC2217_PURGE_TRANSMIT = 1,     //!< Request to purge transmit buffer
    RFC2217_PURGE_BOTH = 2          //!< Request to purge both receive and transmit buffers
} rfc2217_purge_t;

/**
 * @brief baudrate change request callback
 *
 * @param ctx context pointer passed to rfc2217_server_create
 * @param requested_baudrate requested baudrate
 * @return actual baudrate that was set
 */
typedef unsigned (*rfc2217_on_baudrate_t)(void *ctx, unsigned requested_baudrate);

/**
 * @brief control signal change request callback
 *
 * @param ctx context pointer passed to rfc2217_server_create
 * @param requested_control requested control signals
 * @return actual control signals that were set
 */
typedef rfc2217_control_t (*rfc2217_on_control_t)(void *ctx, rfc2217_control_t requested_control);

/**
 * @brief buffer purge request callback
 *
 * @param ctx context pointer passed to rfc2217_server_create
 * @param requested_purge requested buffer purge
 * @return actual buffer purge that was performed
 */
typedef rfc2217_purge_t (*rfc2217_on_purge_t)(void *ctx, rfc2217_purge_t requested_purge);

/**
 * @brief callback on client connection
 * @param ctx context pointer passed to rfc2217_server_create
 */
typedef void (*rfc2217_on_client_connected_t)(void *ctx);

/**
 * @brief callback on client disconnection
 * @param ctx context pointer passed to rfc2217_server_create
 */
typedef void (*rfc2217_on_client_disconnected_t)(void *ctx);

/**
 * @brief callback on data received from client
 * @param ctx context pointer passed to rfc2217_server_create
 * @param data pointer to received data
 * @param len length of received data
 */
typedef void (*rfc2217_on_data_received_t)(void *ctx, const uint8_t *data, size_t len);

/**
 * @brief RFC2217 server configuration
 */
typedef struct {
    void *ctx;  //!< context pointer passed to callbacks
    rfc2217_on_client_connected_t on_client_connected;  //!< callback called when client connects
    rfc2217_on_client_disconnected_t on_client_disconnected;    //!< callback called when client disconnects
    rfc2217_on_baudrate_t on_baudrate;  //!< callback called when client requests baudrate change
    rfc2217_on_control_t on_control;    //!< callback called when client requests control signal change
    rfc2217_on_purge_t on_purge;    //!< callback called when client requests buffer purge
    rfc2217_on_data_received_t on_data_received;    //!< callback called when data is received from client
    unsigned port;              //!< TCP port to listen on
    unsigned task_stack_size;   //!< server task stack size
    unsigned task_priority;     //!< server task priority
    unsigned task_core_id;      //!< server task core ID
} rfc2217_server_config_t;


/** @brief Create RFC2217 server instance
 *
 * @param config RFC2217 server configuration
 * @param out_server pointer to store created server instance
 * @return 0 on success, negative error code on failure
 */
int rfc2217_server_create(const rfc2217_server_config_t *config, rfc2217_server_t *out_server);

/** @brief Start RFC2217 server
 *
 * @param server RFC2217 server instance
 * @return 0 on success, negative error code on failure
 */
int rfc2217_server_start(rfc2217_server_t server);

/** @brief Send data to client
 *
 * @param server RFC2217 server instance
 * @param data pointer to data to send
 * @param len length of data to send
 * @return 0 on success, negative error code on failure
 */
int rfc2217_server_send_data(rfc2217_server_t server, const uint8_t *data, size_t len);

/** @brief Stop RFC2217 server
 *
 * @param server RFC2217 server instance
 * @return 0 on success, negative error code on failure
 */
int rfc2217_server_stop(rfc2217_server_t server);

/** @brief Destroy RFC2217 server instance
 *
 * @param server RFC2217 server instance
 */
void rfc2217_server_destroy(rfc2217_server_t server);


#ifdef __cplusplus
};
#endif
