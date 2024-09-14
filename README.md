# RFC2217 server library for ESP-IDF

[![Component Registry](https://components.espressif.com/components/igrr/rfc2217-server/badge.svg)](https://components.espressif.com/components/igrr/rfc2217-server)

This library provides an RFC2217 server for ESP-IDF. RFC2217 is one of the protocols for accessing serial ports remotely over network. RFC2217 is based on Telnet, extending it with a few commands related to serial port settings.

With this library, you can build a "remote serial port" type of device using an Espressif chip.

## Examples

## Using the component

If you have an existing ESP-IDF project you can run the following command to install the component:
```bash
idf.py add-dependency "igrr/rfc2217-server"
```

## License

This component is provided under Apache 2.0 license, see [LICENSE](LICENSE.md) file for details.
