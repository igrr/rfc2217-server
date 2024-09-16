# API Reference

## Header files

- [rfc2217_server.h](#file-rfc2217_serverh)

## File rfc2217_server.h





## Structures and Types

| Type | Name |
| ---: | :--- |
| enum  | [**rfc2217\_control\_t**](#enum-rfc2217_control_t)  <br>_RFC2217 control signal definitions FIXME: split this into separate enums and callbacks._ |
| typedef unsigned(\* | [**rfc2217\_on\_baudrate\_t**](#typedef-rfc2217_on_baudrate_t)  <br>_baudrate change request callback_ |
| typedef void(\* | [**rfc2217\_on\_client\_connected\_t**](#typedef-rfc2217_on_client_connected_t)  <br>_callback on client connection_ |
| typedef void(\* | [**rfc2217\_on\_client\_disconnected\_t**](#typedef-rfc2217_on_client_disconnected_t)  <br>_callback on client disconnection_ |
| typedef rfc2217\_control\_t(\* | [**rfc2217\_on\_control\_t**](#typedef-rfc2217_on_control_t)  <br>_control signal change request callback_ |
| typedef void(\* | [**rfc2217\_on\_data\_received\_t**](#typedef-rfc2217_on_data_received_t)  <br>_callback on data received from client_ |
| typedef rfc2217\_purge\_t(\* | [**rfc2217\_on\_purge\_t**](#typedef-rfc2217_on_purge_t)  <br>_buffer purge request callback_ |
| enum  | [**rfc2217\_purge\_t**](#enum-rfc2217_purge_t)  <br>_RFC2217 purge request definitions._ |
| struct | [**rfc2217\_server\_config\_t**](#struct-rfc2217_server_config_t) <br>_RFC2217 server configuration._ |
| typedef struct rfc2217\_server\_s \* | [**rfc2217\_server\_t**](#typedef-rfc2217_server_t)  <br>_RFC2217 server instance handle._ |

## Functions

| Type | Name |
| ---: | :--- |
|  int | [**rfc2217\_server\_create**](#function-rfc2217_server_create) (const [**rfc2217\_server\_config\_t**](#struct-rfc2217_server_config_t) \*config, rfc2217\_server\_t \*out\_server) <br>_Create RFC2217 server instance._ |
|  void | [**rfc2217\_server\_destroy**](#function-rfc2217_server_destroy) (rfc2217\_server\_t server) <br>_Destroy RFC2217 server instance._ |
|  int | [**rfc2217\_server\_send\_data**](#function-rfc2217_server_send_data) (rfc2217\_server\_t server, const uint8\_t \*data, size\_t len) <br>_Send data to client._ |
|  int | [**rfc2217\_server\_start**](#function-rfc2217_server_start) (rfc2217\_server\_t server) <br>_Start RFC2217 server._ |
|  int | [**rfc2217\_server\_stop**](#function-rfc2217_server_stop) (rfc2217\_server\_t server) <br>_Stop RFC2217 server._ |


## Structures and Types Documentation

### enum `rfc2217_control_t`

_RFC2217 control signal definitions FIXME: split this into separate enums and callbacks._
```c
enum rfc2217_control_t {
    RFC2217_CONTROL_SET_NO_FLOW_CONTROL = 1,
    RFC2217_CONTROL_SET_XON_XOFF_FLOW_CONTROL = 2,
    RFC2217_CONTROL_SET_HARDWARE_FLOW_CONTROL = 3,
    RFC2217_CONTROL_SET_BREAK = 5,
    RFC2217_CONTROL_CLEAR_BREAK = 6,
    RFC2217_CONTROL_SET_DTR = 8,
    RFC2217_CONTROL_CLEAR_DTR = 9,
    RFC2217_CONTROL_SET_RTS = 11,
    RFC2217_CONTROL_CLEAR_RTS = 12
};
```

### typedef `rfc2217_on_baudrate_t`

_baudrate change request callback_
```c
typedef unsigned(* rfc2217_on_baudrate_t) (void *ctx, unsigned requested_baudrate);
```


**Parameters:**


* `ctx` context pointer passed to rfc2217\_server\_create 
* `requested_baudrate` requested baudrate 


**Returns:**

actual baudrate that was set
### typedef `rfc2217_on_client_connected_t`

_callback on client connection_
```c
typedef void(* rfc2217_on_client_connected_t) (void *ctx);
```


**Parameters:**


* `ctx` context pointer passed to rfc2217\_server\_create
### typedef `rfc2217_on_client_disconnected_t`

_callback on client disconnection_
```c
typedef void(* rfc2217_on_client_disconnected_t) (void *ctx);
```


**Parameters:**


* `ctx` context pointer passed to rfc2217\_server\_create
### typedef `rfc2217_on_control_t`

_control signal change request callback_
```c
typedef rfc2217_control_t(* rfc2217_on_control_t) (void *ctx, rfc2217_control_t requested_control);
```


**Parameters:**


* `ctx` context pointer passed to rfc2217\_server\_create 
* `requested_control` requested control signals 


**Returns:**

actual control signals that were set
### typedef `rfc2217_on_data_received_t`

_callback on data received from client_
```c
typedef void(* rfc2217_on_data_received_t) (void *ctx, const uint8_t *data, size_t len);
```


**Parameters:**


* `ctx` context pointer passed to rfc2217\_server\_create 
* `data` pointer to received data 
* `len` length of received data
### typedef `rfc2217_on_purge_t`

_buffer purge request callback_
```c
typedef rfc2217_purge_t(* rfc2217_on_purge_t) (void *ctx, rfc2217_purge_t requested_purge);
```


**Parameters:**


* `ctx` context pointer passed to rfc2217\_server\_create 
* `requested_purge` requested buffer purge 


**Returns:**

actual buffer purge that was performed
### enum `rfc2217_purge_t`

_RFC2217 purge request definitions._
```c
enum rfc2217_purge_t {
    RFC2217_PURGE_RECEIVE = 0,
    RFC2217_PURGE_TRANSMIT = 1,
    RFC2217_PURGE_BOTH = 2
};
```

### struct `rfc2217_server_config_t`

_RFC2217 server configuration._

Variables:

-  void \* ctx  <br>_context pointer passed to callbacks_

-  rfc2217\_on\_baudrate\_t on_baudrate  <br>_callback called when client requests baudrate change_

-  rfc2217\_on\_client\_connected\_t on_client_connected  <br>_callback called when client connects_

-  rfc2217\_on\_client\_disconnected\_t on_client_disconnected  <br>_callback called when client disconnects_

-  rfc2217\_on\_control\_t on_control  <br>_callback called when client requests control signal change_

-  rfc2217\_on\_data\_received\_t on_data_received  <br>_callback called when data is received from client_

-  rfc2217\_on\_purge\_t on_purge  <br>_callback called when client requests buffer purge_

-  unsigned port  <br>_TCP port to listen on._

-  unsigned task_core_id  <br>_server task core ID_

-  unsigned task_priority  <br>_server task priority_

-  unsigned task_stack_size  <br>_server task stack size_

### typedef `rfc2217_server_t`

_RFC2217 server instance handle._
```c
typedef struct rfc2217_server_s* rfc2217_server_t;
```


## Functions Documentation

### function `rfc2217_server_create`

_Create RFC2217 server instance._
```c
int rfc2217_server_create (
    const rfc2217_server_config_t *config,
    rfc2217_server_t *out_server
) 
```


**Parameters:**


* `config` RFC2217 server configuration 
* `out_server` pointer to store created server instance 


**Returns:**

0 on success, negative error code on failure
### function `rfc2217_server_destroy`

_Destroy RFC2217 server instance._
```c
void rfc2217_server_destroy (
    rfc2217_server_t server
) 
```


**Parameters:**


* `server` RFC2217 server instance
### function `rfc2217_server_send_data`

_Send data to client._
```c
int rfc2217_server_send_data (
    rfc2217_server_t server,
    const uint8_t *data,
    size_t len
) 
```


**Parameters:**


* `server` RFC2217 server instance 
* `data` pointer to data to send 
* `len` length of data to send 


**Returns:**

0 on success, negative error code on failure
### function `rfc2217_server_start`

_Start RFC2217 server._
```c
int rfc2217_server_start (
    rfc2217_server_t server
) 
```


**Parameters:**


* `server` RFC2217 server instance 


**Returns:**

0 on success, negative error code on failure
### function `rfc2217_server_stop`

_Stop RFC2217 server._
```c
int rfc2217_server_stop (
    rfc2217_server_t server
) 
```


**Parameters:**


* `server` RFC2217 server instance 


**Returns:**

0 on success, negative error code on failure


