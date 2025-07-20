#!/bin/bash
# This uses a single curl process with multiple URLs
URL="http://localhost:8080/api/time"

# Build URL list for single curl command
urls=""
for i in $(seq 1 50); do
    urls="$urls $URL"
done

echo "Testing Keep-Alive with single curl session..."
time curl -s $urls > /dev/null

echo "Testing without Keep-Alive (separate connections)..."  
time for i in $(seq 1 50); do curl -s -o /dev/null $URL; done
