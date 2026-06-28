# v1.21.1

## what's new

Small follow-up fix found right after publishing v1.21.0.

### Remote signals are logged again regardless of MQTT state

`mqttSendRemote()` returned immediately if MQTT was not connected (or disabled), which skipped the "received remote signal" log line entirely - so remote button presses no longer showed up in the log unless MQTT happened to be connected at that exact moment. The log line is now always written; only the MQTT status publish is skipped when there is no broker connection.

## changelog

- [FIX] Remote-control commands are logged again even when MQTT is disabled or disconnected
