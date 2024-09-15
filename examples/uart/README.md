# RFC2217 UART Example

This example sets up an RFC2217 server, accepts client connection, and sends any data received to UART. Bytes received from UART are sent to the RFC2217 client. When the client disconnects, the server waits for a new connection.

The example can be used with any ESP chip. You need to connect a USB-to-serial adapter to the UART port of the ESP board to test the example.

Network connection is achieved using `protocols_examples_common` component, which provides a simple API for connecting to Wi-Fi or Ethernet. You can configure the connection method and the credentials in menuconfig.

## How to Use the Example

Build and flash the example as usual. For example, when using an ESP32-S3 chip:

```shell
idf.py set-target esp32s3
idf.py flash monitor
```

By default, the example uses UART1, GPIO 4 for TX and GPIO 5 for RX. If you want to use a different UART or different GPIOs, you can change the configuration in menuconfig.

Note the IP address printed on the console when the example runs. You will need it to connect to the device. For example:
```
I (4547) example_common: Connected to example_netif_sta
I (4557) example_common: - IPv4 address: 192.168.0.196,
```

After flashing, the device will start an RFC2217 server. You can connect to it using an RFC2217 client. Since you have IDF already installed, the easiest way is to use `miniterm` from pySerial (replace the IP address with the one you noted earlier):
    
```shell
python -m serial.tools.miniterm rfc2217://192.168.0.180:3333 115200
```

Connect the USB-to-serial adapter to the UART port of the ESP board (RX to TX, TX to RX, GND to GND).

Open another terminal and connect to the USB-to-serial adapter using `miniterm` or any other terminal emulator. For example, if the USB-to-serial adapter is connected to `/dev/ttyUSB1`:
```shell
python -m serial.tools.miniterm /dev/ttyUSB1 115200
```

Characters typed into the first miniterm window will be sent to the server, forwarded to UART, and will appear in the second miniterm window. Same goes for the opposite direction.

The example doesn't echo the typed characters to the console of the ESP chip (UART0 or USB_SERIAL_JTAG), but you can modify the code to do that if needed.

To exit miniterm, press `Ctrl+]`.

## Example output

```
I (4565) example_common: - IPv4 address: 192.168.0.180,
I (4565) example_common: - IPv6 address: fe80:0000:0000:0000:1206:1cff:fe98:49dc, type: ESP_IP6_ADDR_IS_LINK_LOCAL
I (4585) app_main: Starting RFC2217 server on port 3333
I (4585) app_main: Waiting for client to connect
I (9245) rfc2217_server: TCP receive thread started, socket: 55
I (9255) app_main: Client connected, starting data transfer
I (48895) rfc2217_server: Connection closed
I (48895) rfc2217_server: TCP receive thread done
I (48915) app_main: Client disconnected
I (48915) app_main: Waiting for client to connect
```
