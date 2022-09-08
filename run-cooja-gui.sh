#!/bin/bash

CURDIR=`pwd`
SCRIPT_PATH=${0%/*}

if [ "$0" != "$SCRIPT_PATH" ] && [ "$SCRIPT_PATH" != "" ] && [ "$SCRIPT_PATH" != "." ]; then
    COOJA_PATH=$SCRIPT_PATH/tools/cooja
    CONTIKI_PATH=$CURDIR/$SCRIPT_PATH/tools/contiki-ng
else
    COOJA_PATH=tools/cooja
    CONTIKI_PATH=$CURDIR/tools/contiki-ng
fi

if [ -z "$1" ]; then
    ARGS=""
else
    ARGS="-quickstart=$CURDIR/$1"
fi

$COOJA_PATH/gradlew -p $COOJA_PATH run --args="$ARGS -contiki=$CONTIKI_PATH -logdir=$CURDIR"
