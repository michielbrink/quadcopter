#!/bin/bash
until python client.py; do
    echo "'client.py' crashed with exit code $?. Restarting..." >&2
    sleep 1
done
