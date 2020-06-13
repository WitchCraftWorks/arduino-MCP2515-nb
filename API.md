# CAN MCP2515-nb API

## Include Library

```arduino
#include <MCP2515_nb.h>
```

## Setup

Define an instance and initialize it.

```arduino
// (somewhere outside any function and at the top of your sketch after include)
MCP2515 MCP = MCP2515();

// We will assume MCP is the variable with a MCP2515 instance for every snippet
```

## Begin

Starts the SPI and initializes the CAN controller.

```arduino
// in setup()
MCP.begin(baudrate);
```
 * `baudrate` - The CAN bus baud rate, i.e. `MCP2515_CAN_SPEED::50KBPS` or `50e6`.

Returns a `MCP2515_ERRORCODES` enum integer (all errors are negative integers).

### Errors

| Error enum | Integer value | Description |
| ---------- | ------------- | ----------- |
| OK         | 0             | Operation was successful |
| BADF       | -9            | Unable to put controller into specific mode |
| INVAL      | -22           | Invalid value/argument, check baudrate validity |

## End

Stops the SPI and resets the controller.

```arduino
MCP.end();
```

Returns void.

## Set pins

Override the default `CS` and `INT` pins used by the library. **Must** be called before `MCP.begin(...)`.

```arduino
MCP.setPins(cs, irq);
```
 * `cs` - New chip select pin to use, defaults to `10`.
 * `irq` - New INT pin to use, defaults to `2`.  **Must** be interrupt capable via [attachInterrupt(...)](https://www.arduino.cc/en/Reference/AttachInterrupt).

This call is optional and only needs to be used if you need to change the default pins used.

## Set SPI Frequency

Override the default SPI frequency of 10 MHz used by the library. **Must** be called before `MCP.begin(...)`.

```arduino
MCP.setSPIFrequency(frequency);
```
 * `frequency` - New SPI frequency to use, defaults to `10e6`.

This call is optional and only needs to be used, if you need to change the default SPI frequency used.
Some logic level converters cannot support high speeds such as 10 MHz, so a lower SPI frequency can be selected with `MCP.setSPIFrequency(frequency)`.

## Set Clock Frequency

Override the default clock source frequency that is connected to the MCP2515. **Must** be called before `MCP.begin(...)`.

```arduino
MCP.setClockFrequency(clockFrequency);
```
 * `clockFrequency` - New clock frequency to use (`8e6`, `16e6`) connected to MCP2515, defaults to 16 MHz.

This call is optional and only needs to be used if you need to change the clock source frequency connected to the MCP2515.
Most shields have a 16 MHz clock source on board, some breakout boards have a 8 MHz source.

## Filtering

Filter packets that meet the desired criteria.

```
MCP.filter(id);
MCP.filter(id, mask);

MCP.filterExtended(id);
MCP.filterExtended(id, mask);

MCP.multiFilter(ids, count);
```

 * `id` - 11-bit id (standard packet) or 29-bit packet id (extended packet).
 * `mask` - (optional) 11-bit mask (standard packet) or 29-bit mask (extended packet), defaults to `0x7FF` or `0x1FFFFFFF` (extended).
 * `ids` - An array of ints to use as id filter.
 * `count` - A counter for the ids array (max. 6).

Only packets that meet the following criteria are acknowleged and received, other packets are ignored:

```
if ((packetId & mask) == id) {
  // acknowleged and received
} else {
  // ignored
}
```

Returns a `MCP2515_ERRORCODES` enum integer (all errors are negative integers).

### Errors

| Error enum | Integer value | Description |
| ---------- | ------------- | ----------- |
| OK         | 0             | Operation was successful |
| BADF       | -9            | Unable to put controller into specific mode |
| INVAL      | -22           | Invalid value/argument |

## Receive packet

You can receive a packet (if there is one) by calling `MCP.receivePacket(*packet)`.

```arduino
MCP.receivePacket(*packet);
```
* `*packet` - A pointer to a `CANPacket` instance.

Returns a `MCP2515_ERRORCODES` enum integer (all errors are negative integers).

### Errors

| Error enum | Integer value | Description |
| ---------- | ------------- | ----------- |
| OK         | 0             | Message was received |
| ENOENT     | -2            | No message pending |

## Receive packet with callback (interrupts)

The CAN controller and Arduino boards support interrupts. This library offers support for RX and TX interrupts (receiving a CAN message and sent a CAN message).

When a new message arrives, the given callback will be invoked with a `CANPacket` pointer.

```arduino
MCP.onReceivePacket(onReceive);

void onReceive(CANPacket* packet) {
	// do something with packet
	// note that you should copy the packet to your "namespace" if you wish to use it for longer than this function call
}
```

The interrupt will be automatically installed and the CAN controller instructed to output interrupts.
The following interrupts will be enabled: RX, TX, MERR (message error), WAK (wakeup).
Which means the CAN controller can be used to wakeup the MCU when bus activity is detected (CAN controller needs to be put to sleep).

**Note: Currently only SAMD supports multiple MCP instances (one MCP per interrupt pin)! All other architectures support only ONE MCP instance globally.**

## Sending packet

You can send a packet by calling `MCP.writePacket(*packet, nowait)`. This operation can be made non-blocking by setting the second parameter to `true`.
When `nowait` is set to `true`, the packet will be put into a TX queue. If the CAN controller's TX buffer is empty, the packet will be immediately written to it.

You should make sure that the instance does not get freed before the complete write operation is completed, especially when using `nowait`.
This can easily escalate into a segfault.

When not using interrupts, you should call `MCP.processTxQueue()` periodically to process the outgoing packet queue.
The current packet queue size/length can be acquired using `MCP.getTxQueueLength()` (returns a `size_t`).

If sending the packet fails, the packet will NOT be automatically removed from the TX buffer on bus transmission errors (return value = `BADF`).
You need to either set the one-shot mode (which will make the CAN controller remove the packet automatically) or call `abortPacket` (if nowait = false).

If the packet stays in the TX buffer, the CAN controller will automatically try to send the packet. The `CANPacket` will however not be updated.

```arduino
MCP.sendPacket(*packet, nowait);
```
* `*packet` - A pointer to a `CANPacket` instance.
* `nowait` - A boolean indicating whether the packet should be sent non-blocking (not waiting for confirmation), defaults to `false`.

Returns a `MCP2515_ERRORCODES` enum integer (all errors are negative integers).

### Errors

| Error enum | Integer value | Description |
| ---------- | ------------- | ----------- |
| OK         | 0             | Message was successfully written or queued |
| INTR       | -4            | Message was successfully aborted (abort requested by user, nowait = false only) |
| BADF       | -9            | Message has encountered bus transmission error (nowait = false only) |
| BUSY       | -16           | Message can not be sent as CAN controller is busy with another message (nowait = false only) |
| INVAL      | -22           | Invalid message (did you end the packet?) |
| COMM       | -70           | Controller is not in normal or loopback mode (listen, sleep or config mode active) |
| OVERFLOW   | -75           | TX queue is full (nowait = true only) |

## Abort packet

Any outgoing packet that has not yet been written to the CAN bus, can be requested to be aborted.
If the async TX queue is empty, any packet that is in the TX buffer will be requested to be aborted (this may the last packet that was tried to be written synchronously).

```arduino
MCP.abortPacket(*packet, nowait);
```
* `*packet` - A pointer to a `CANPacket` instance.
* `nowait` - A boolean indicating whether the packet should be abort non-blocking (not waiting for confirmation), defaults to `false`.

Returns a `MCP2515_ERRORCODES` enum integer (all errors are negative integers).

### Errors

| Error enum | Integer value | Description |
| ---------- | ------------- | ----------- |
| OK         | 0             | Message was successfully aborted (nowait = true) or abort was requested (nowait = false) |
| BADF       | -9            | Message could not have been aborted |

## Packet Status

Each `CANPacket` has a `status` field (`packet.getStatus()`), which contains a bitfield of `CANPacket::STATUS_*` constants as `unsigned long`.

Currently implemented status constants:
| Name                      | Value   | Description |
| ------------------------- | ------- | ----------- |
| STATUS_RX_OK              | 1       | Incoming message ok |
| STATUS_RX_INVALID_MESSAGE | 2       | Incoming message might be invalied (only in listen-only mode and `allowInvalid` = true) |
| STATUS_TX_PENDING         | 16      | Outgoing message is pending for write |
| STATUS_TX_SENT            | 128     | Outgoing message was sent |
| STATUS_TX_ABORT_REQUESTED | 1024    | Abort was requested |
| STATUS_TX_ABORTED         | 2048    | Message was aborted |
| STATUS_TX_ERROR           | 16384   | Message encountered a bus transmission error |

The user can check the status field using binary operations and add certain logic when something specific happened.

## Wait for packet status

The user can use the provided `waitForPacketStatus` to wait until a packet has reached a certain status (non-blocking enabled).

```arduino
MCP.waitForPacketStatus(*packet, status, nowait, timeout);
```
* `*packet` - A pointer to a `CANPacket` instance.
* `status` - A long indicating which status to check for.
* `nowait` - A boolean indicating whether the function should only check once and then return, defaults to `false`.
* `timeout` - A timeout in milliseconds when `nowait` = false, defaults to `0` for no timeout.

### Errors

| Error enum | Integer value | Description |
| ---------- | ------------- | ----------- |
| OK         | 0             | Message was successfully reached given status |
| BADF       | -9            | Message has not reached status and message sending failed somehow |
| AGAIN      | -11           | Operation timed out (timeout > 0) or would've blocked (nowait = true) |
| INVAL      | -22           | Invalid message (incoming message) |

## Operation modes

The CAN controller can be put into different operation modes.

## Listen-Only mode

Put the CAN controller in Listen-Only mode, this mode provides a means to receive all messages (including messages with errors).
Listen-Only mode is a silent mode, meaning no messages will be transmitted while in this mode (including error flags or acknowledge signals).

```arduino
MCP.setListenMode(allowInvalid);
```
* `allowInvalid` - allows the controller to receive errornous messages.

The `allowInvalid` parameter is interesting for bus diagnostics. The parameter will make the CAN controller accept all messages, regardless of acceptance masks and filters.

Returns a `MCP2515_ERRORCODES` enum integer (all errors are negative integers).

### Errors

| Error enum | Integer value | Description |
| ---------- | ------------- | ----------- |
| OK         | 0             | Operation was successful |
| BADF       | -9            | Unable to put controller into specific mode |

## Loopback mode

Put the CAN controller into loopback mode, any outgoing packets will also be received. No other packets can be received.

```arduino
MCP.setLoopbackMode();
```

Returns a `MCP2515_ERRORCODES` enum integer (all errors are negative integers).

### Errors

| Error enum | Integer value | Description |
| ---------- | ------------- | ----------- |
| OK         | 0             | Operation was successful |
| BADF       | -9            | Unable to put controller into specific mode |

## Sleep mode

Put the CAN contoller into sleep mode.

```arduino
MCP.setSleepMode();
```

Returns a `MCP2515_ERRORCODES` enum integer (all errors are negative integers).

### Errors

| Error enum | Integer value | Description |
| ---------- | ------------- | ----------- |
| OK         | 0             | Operation was successful |
| BADF       | -9            | Unable to put controller into specific mode |

## Normal mode

This will put the CAN controller into normal operation mode. Wake up the CAN contoller, if it was previously in sleep mode.

```arduino
MCP.setNormalMode();
```

Returns a `MCP2515_ERRORCODES` enum integer (all errors are negative integers).

### Errors

| Error enum | Integer value | Description |
| ---------- | ------------- | ----------- |
| OK         | 0             | Operation was successful |
| BADF       | -9            | Unable to put controller into specific mode |

## Wakeup Filter

The CAN controller is actively listening in sleep mode for bus activity.
The CAN controller may wakeup during short glitches on the bus.

A low-pass filter function can be enabled to prevent wakeups during short glitches.

```arduino
MCP.setWakeupFilter(enable);
```
* `enable` - A boolean indicating whether to enable or disable the filter.

Returns a `MCP2515_ERRORCODES` enum integer (all errors are negative integers).

### Errors

| Error enum | Integer value | Description |
| ---------- | ------------- | ----------- |
| OK         | 0             | Operation was successful |
| BADF       | -9            | Unable to put controller into specific mode |

## Disabling async TX queue

The TX queue can be disabled by defining `MCP2515_DISABLE_ASYNC_TX_QUEUE` before `include`ing the library. This will remove at compile-time the queue used for sending `CANPacket`s asynchronously.

For example:
```arduino
#define MCP2515_DISABLE_ASYNC_TX_QUEUE 1
#include <MCP2515_nb.h>
```

With the following changes come along with it:
* `getTxQueueLength` will always return `0`.
* `processTxQueue` does nothing.
* When using `writePacket` with `nowait = true` the library will return `MCP2515_ERRORCODES::AGAIN` (-11) if the TX buffer is already in use.
* When using `abortPacket` any packet that is currently in the TX buffer will be aborted, regardless of the given `CANPacket`.
