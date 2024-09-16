# RFC2217 USB CDC Example

This example demonstrates a bridge between an RFC2217 server and a USB CDC device. ESP chip acts as a USB Host and an RFC2217 server at the same time. Data received from the network using RFC2217 protocol is sent to the USB CDC device, and data received from the USB CDC device is sent to the network using RFC2217 protocol. With this example, you can get remote access to a USB CDC device over the network.

The example can be used with any ESP chip with USB_OTG peripheral (currently ESP32-S2, ESP32-S3, ESP32-P4). The development board needs to have a USB host port to which a USB CDC device (e.g. a USB-to-UART adapter) can be connected.

Network connection is achieved using `protocols_examples_common` component, which provides a simple API for connecting to Wi-Fi or Ethernet. You can configure the connection method and the credentials in menuconfig.

## How to Use the Example

Build and flash the example as usual. For example, when using an ESP32-P4 chip:

```shell
idf.py set-target esp32p4
idf.py flash monitor
```

Note the IP address printed on the console when the example runs. You will need it to connect to the device. For example:
```
I (4547) example_common: Connected to example_netif_sta
I (4557) example_common: - IPv4 address: 192.168.0.196,
```

After flashing, the device will start an RFC2217 server. You can connect to it using an RFC2217 client. Since you have IDF already installed, the easiest way is to use `miniterm` from pySerial (replace the IP address with the one you noted earlier):
    
```shell
python -m serial.tools.miniterm rfc2217://192.168.0.180:3333 115200
```

Plug the USB-to-serial adapter into the USB host port of the ESP board. On ESP32-P4-Function-EVB, the USB A port is the USB host port. On other boards, you may need to wire up the port yourself. Check ESP-IDF USB Host examples for more information.

Open another terminal and connect to the USB-to-serial adapter using `miniterm` or any other terminal emulator. For example, if the USB-to-serial adapter is connected to `/dev/ttyUSB1`:
```shell
python -m serial.tools.miniterm /dev/ttyUSB1 115200
```

Characters typed into the first miniterm window will be sent to the server, forwarded to USB CDC, and will appear in the second miniterm window. Same goes for the opposite direction.

The example doesn't echo the typed characters to the console of the ESP chip (UART0 or USB_SERIAL_JTAG), but you can modify the code to do that if needed.

To exit miniterm, press `Ctrl+]`.

## Example output

```
I (3156) example_common: Connected to example_netif_eth
I (3156) example_common: - IPv4 address: 192.168.0.125,
I (3156) example_common: - IPv6 address: fe80:0000:0000:0000:6255:f9ff:fef9:045d, type: ESP_IP6_ADDR_IS_LINK_LOCAL
I (3156) VCP example: Installing USB Host
I (3186) VCP example: Installing CDC-ACM driver
I (3186) app_main: Starting RFC2217 server on port 3333
I (3186) VCP example: Opening VCP device...
I (3836) VCP example: Setting up line coding
I (3836) app_main: USB Device connected
I (6596) rfc2217_server: TCP receive thread started, socket: 55
I (6596) app_main: RFC2217 client connected
I (6646) VCP example: Setting baud rate to 115200
I (12996) rfc2217_server: Connection closed
I (12996) app_main: RFC2217 client disconnected
I (12996) rfc2217_server: TCP receive thread done
```
