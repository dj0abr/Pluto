# Pluto
easy to use RX and TX handler for the Adalm - Pluto

supported :

- Pluto via USB connection
- Pluto via Ethernet/USB adapter
- full networking 
- easy configuration in one file: pluto.h


## for LINUX only

running on almost any Linux distribution and
on small SBCs (Raspberry, Odroid..) as long as the SBC is fast enough to handle the sample rates.

Why just Linux ? 
Because its one of my holy rules to NEVER use proprietary operating systems infested with ads and trackers which are sending my personal data around the globe.

## Overview

This program connects to a Pluto via USB or Ethernet.

If a Pluto is found, it will be initialized for RX and TX. If init is ok, streaming starts immediately.
This program does all work with the pluto itself, no need to have any understanding of the Pluto or its libraries.

This program runs as a separate thread. All data from/to the user program are streamed via an UDP port.

The user application can be written in any possible language, the only requirement is to send and receive UDP messages.

This eliminates the need for the application programmer to understand how the Pluto is internally working.

## Complete Networking Support

The Pluto may be connected via USB or Ethernet

The application talks to this program via Ethernet (via cable or local).

So you can place the Pluto somewhere, connect it to ethernet, place this driver somewhere else and place your application even in another place. Or everything in one machine. There is no restriction.

## Prerequisites

to install all required libraries run: ./prepare_ubuntu_pluto

## Usage

1) Configure the SDR parameters

open file pluto.h and look for the section: INITIALISATION START. Enter all settings as required. (Tipp: if you enter the same UDP port for RX and TX then pluto will send what it receives, and therefore acts as a repeater. You have to choose different RX/TX freuquencies in this mode).

2) compile the program: make 

3) run the program (as a normal user):  ./pluto

## Interface to external applications

samples are sent to this program (or received) via UDP messages. A message MUST have the length of 32768 bytes with this format:
```
Byte 0 ... I-sample LSB
Byte 1 ... I-sample MSB
Byte 2 ... Q-sample LSB
Byte 3 ... Q-sample MSB
```
## Synchronisation

In an SDR transmitter the sample rate is defined by the SDR hardware. Therefore the applications needs to know when to send new samples to this program.

A FIFO is located between the UDP interface and the Pluto's transmitter. By checking the fill state of this fifo an application can see when to send new data.

This program sends the following UDP message every second:
```
Port: UDP_STATUSPORT
Byte 0: number of frames currently in the fifo (MSB)
Byte 1
Byte 2
Byte 3: LSB
```
We get a TX underrun if this number goes to 0. The application has to check this number and send new frames (with 32768 bytes each, see above) if this number goes below i.e. 5 or 10. You can experiment how low you can go to reduce latency. If its too low, underrun will happen resulting in transmission losses.
