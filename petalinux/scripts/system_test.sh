#!/bin/bash

echo "Pulse Radar System Test"
echo "======================"

# Check if radar device exists
if [ ! -e /dev/pulse_radar_ip ]; then
    echo "ERROR: Radar device not found"
    exit 1
fi

echo "✓ Radar device found"

# Check driver loading
if ! lsmod | grep -q radar_driver; then
    echo "ERROR: Radar driver not loaded"
    exit 1
fi

echo "✓ Radar driver loaded"

# Check sysfs attributes
if [ -e /sys/class/misc/pulse_radar_ip/prf ]; then
    echo "✓ Sysfs attributes available"
    echo "  Current PRF: $(cat /sys/class/misc/pulse_radar_ip/prf)"
    echo "  Current pulse width: $(cat /sys/class/misc/pulse_radar_ip/pulse_width)"
    echo "  Current status: $(cat /sys/class/misc/pulse_radar_ip/status)"
else
    echo "WARNING: Sysfs attributes not found"
fi

# Test basic radar functionality
echo "Testing radar functionality..."
radar_app -p 2000 -w 10 -t 100 -s

echo "✓ Radar test completed"

# Test target detection (simulation)
echo "Testing target detection (10 seconds)..."
timeout 10 radar_app -m

echo "✓ System test completed successfully"
