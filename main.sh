#!/bin/bash

echo "Starting Hospital Management System in separate terminals..."



# For gnome-terminal (Ubuntu/Debian)
gnome-terminal -- bash -c "./patient; exec bash" &
sleep 2

gnome-terminal -- bash -c "./lab; exec bash" &
sleep 2

gnome-terminal -- bash -c "./ui; exec bash" &
sleep 2


# For xterm (alternative)
# xterm -e "./patient" &
# sleep 2
# xterm -e "./lab" &
# sleep 2
# xterm -e "./ui" &
# sleep 2
# xterm -e "./main" &

echo "All services started in separate terminals!"
