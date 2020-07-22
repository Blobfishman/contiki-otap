#!/bin/sh
msp430-objcopy package.sky -O ihex package.ihex
objcopy -I ihex -O binary package.ihex package.bin