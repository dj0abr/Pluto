# Pluto
easy to use RX and TX handler for the Adalm - Pluto

## for LINUX only

running on almost any Linux distribution and
on small SBCs (Raspberry, Odroid..) as long as the SBC is fast enough to handle the sample rates.

Why just Linux ? 
Because its one of my holy rules to NEVER use proprietary operating systems.

## this is work in progress
do not use these files before this warning is removed

## Overview

This program connects to a Pluto via USB or Ethernet.

If a Pluto is found, it will be initialized for RX and TX and a couple of handler threads are created.
This program does all work with the pluto itself, no need to have any understanding of the Pluto or its libraries.

This program runs as a separate thread. All data from/to the user program are streamed via an UDP port.

The user application can be written in any possible language, the only requirement is to send and receive UDP messages.

This eliminates the need for the application programmer to understand how the Pluto is internally working.
