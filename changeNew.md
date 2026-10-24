# v1.22.5

## what's new

### Commands rejected after RF noise

Continuous RF noise on the receiver could overrun the receive buffer. This overwrote adjacent memory, including the command queue. Afterwards the controller rejected every new command with "too many commands within too short time", and only a restart helped. The receive buffer is now protected against overrun.

### Command queue watchdog

As an additional safeguard, a command queue that does not shrink for more than 12 seconds is logged as stuck and reset, instead of blocking all further commands. The error "too many commands within too short time" now also shows the queue size.

## changelog

- [FIX] RF noise could overrun the receive buffer and block all further commands
- [FEATURE] watchdog resets a stuck command queue
- [FEATURE] queue size in the "too many commands" log message
