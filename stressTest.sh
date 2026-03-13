#!/bin/bash

NUM_CLIENTS=$1

if [ -z "$NUM_CLIENTS" ]; then
    echo "Usage: ./stressTest.sh <num_clients>"
    exit 1
fi

for ((i=1;i<=NUM_CLIENTS;i++))
do
    (
        # Start the client in the background
        ./client2 &
        pid=$!

        # Calculate random lifetime between 5 and 15 seconds
        lifetime=$((RANDOM % 3+5))

        echo "Client $i running for $lifetime seconds (pid $pid)"

        sleep $lifetime

        # Terminate the client
        kill -SIGINT $pid 2>/dev/null

        echo "Client $i (pid $pid) killed"
    ) &
done

wait
echo "All clients finished"
