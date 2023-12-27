# UART to websocket backend for rbbs

## What is this?

This is a websockets server that:

* Pretends to be a modem
* Allows a single connection at once (and rejects additional ones with `BUSY`)
* For the single connection, all input/output is sent to the specified UART device

## Prerequisites

You'll need to have openssl present and correct on your system. Linux systems 
almost certainly already have it, you might need to `brew install` it on macOS.

`libwebsockets` is used, but will be downloaded and built as part of the build.

## Building

```shell
cmake .
make all
```

Obviously you can do `--build` directly to `cmake` or whatever if you like. 
You do you... 

## Running

```
./rbbs-ws -d /dev/cu.usbmodem14101
```

The default is 115200bps. If your UART is running at 9600, that is
supported to with the `-b 9600` flag. 

Other baud rates are TODO.

## Anything else?

This is super dirty code at this point, and probably full of bugs. It also doesn't
have any kind of security, proper session handling, SSL, or anything else.

So I probably wouldn't go exposing it to the actual Internet just yet...
