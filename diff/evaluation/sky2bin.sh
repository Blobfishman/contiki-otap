#!/bin/sh
msp430-objcopy $1 -O ihex package.ihex
objcopy -I ihex -O binary package.ihex $2
