#!/usr/bin/env bash

if [ -z "$1" ]; then
    echo "$0: Input file is required."
    exit 4
fi

output=$((perl -Mblib $1) 2>&1)
ctor=$(echo "$output" | grep "\[ctor\]" | sort | awk '{print $1 }' | awk -F':' '{print $3}' | uniq -c | sort -n)
dtor=$(echo "$output" | grep "\[dtor\]" | sort | awk '{print $1 }' | awk -F':' '{print $3}' | uniq -c | tr -d '~' | sort -n)
diff <(echo "$ctor") <(echo "$dtor") | colordiff
if [ $? -eq 0 ]; then
    echo "No leaks $1"
else
    echo "Constructed"
    echo "$ctor"
    echo "Annihilated"
    echo "$dtor"
fi
