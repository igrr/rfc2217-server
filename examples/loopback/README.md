# RFC2217 Loopback Example

This example sets up an RFC2217 server, accepts client connection, and echoes back any data received. When the client disconnects, the server waits for a new connection.

The example can be used with any ESP chip, no additional hardware is required.

Network connection is achieved using `protocols_examples_common` component, which provides a simple API for connecting to Wi-Fi or Ethernet. You can configure the connection method and the credentials in menuconfig.

## How to Use the Example

Build and flash the example as usual. For example, when using an ESP32-S3 chip:

```shell
idf.py set-target esp32s3
idf.py flash monitor
```

Note the IP address printed on the console. You will need it to connect to the device. For example:
```
I (4547) example_common: Connected to example_netif_sta
I (4557) example_common: - IPv4 address: 192.168.0.196,
```

After flashing, the device will start an RFC2217 server. You can connect to it using an RFC2217 client. Since you have IDF already installed, the easiest way is to use `miniterm` from pySerial (replace the IP address with the one you noted earlier):
    
```shell
python -m serial.tools.miniterm rfc2217://192.168.0.196:3333
```

Once the client is connected, the server will send a greeting message. Characters typed into `miniterm` will be echoed back by the server.

To exit miniterm, press `Ctrl+]`.

## Example output

```
I (4547) example_common: Connected to example_netif_sta
I (4557) example_common: - IPv4 address: 192.168.0.196,
I (4557) example_common: - IPv6 address: fe80:0000:0000:0000:bedd:c2ff:fed4:a888, type: ESP_IP6_ADDR_IS_LINK_LOCAL
I (4567) app_main: Starting RFC2217 server on port 3333
I (4577) app_main: Waiting for client to connect
I (8257) rfc2217_server: TCP receive thread started, socket: 55
I (8257) app_main: Client connected, sending greeting
I (9257) app_main: Waiting for client to disconnect
I (10497) app_main: Data received: (1 bytes)
I (10607) app_main: Data received: (1 bytes)
I (10707) app_main: Data received: (1 bytes)
I (10907) app_main: Data received: (1 bytes)
I (10997) app_main: Data received: (2 bytes)
I (11217) app_main: Data received: (1 bytes)
I (11227) app_main: Data received: (1 bytes)
I (11317) app_main: Data received: (1 bytes)
I (11507) app_main: Data received: (1 bytes)
I (11727) app_main: Data received: (2 bytes)
I (11767) app_main: Data received: (1 bytes)
I (11857) app_main: Data received: (1 bytes)
I (11907) app_main: Data received: (1 bytes)
I (11997) app_main: Data received: (1 bytes)
I (12017) app_main: Data received: (1 bytes)
I (12047) app_main: Data received: (1 bytes)
I (12077) app_main: Data received: (1 bytes)
I (14497) rfc2217_server: Connection closed
I (14497) rfc2217_server: TCP receive thread done
I (14497) app_main: Client disconnected
I (14497) app_main: Waiting for client to connect
```
