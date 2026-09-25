#!/bin/bash

echo "Hello from STDOUT"
echo "This is an error" >&2
echo "VAR = $VAR"

sleep 1

echo "Another stdout line"
echo "Another error line" >&2

exit 1
