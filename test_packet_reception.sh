#!/bin/bash

# Script to test if packets are being received on AuthServer port
# Usage: ./test_packet_reception.sh

echo "Testing packet reception on port 20210..."
echo "This will show if any data is arriving at the AuthServer port"
echo ""
echo "Press Ctrl+C to stop"
echo ""

# Use tcpdump to monitor packets (requires sudo)
if command -v tcpdump &> /dev/null; then
    echo "Using tcpdump to monitor port 20210..."
    sudo tcpdump -i any -n -X port 20210
elif command -v netstat &> /dev/null; then
    echo "Using netstat to check connections..."
    watch -n 1 "netstat -an | grep 20210"
else
    echo "Error: Neither tcpdump nor netstat found. Install tcpdump for packet monitoring."
    exit 1
fi
