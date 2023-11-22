#!/bin/bash

CURDIR=`pwd`
SCRIPT_PATH=${0%/*}

if [ "$0" != "$SCRIPT_PATH" ] && [ "$SCRIPT_PATH" != "" ] && [ "$SCRIPT_PATH" != "." ]; then
    COOJA_PATH=$SCRIPT_PATH/tools/cooja
else
    COOJA_PATH=./tools/cooja
fi

$COOJA_PATH/scripts/run-cooja.py $@
