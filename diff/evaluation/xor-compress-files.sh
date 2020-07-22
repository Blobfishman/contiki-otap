#!/bin/sh
./Creator-xor bin-output/reference.bin bin-output/small-change/add-code.bin xor-output/small-change/add-code.patch
./Creator-xor bin-output/reference.bin bin-output/small-change/add-print.bin xor-output/small-change/add-print.patch
./Creator-xor bin-output/reference.bin bin-output/small-change/add-print2.bin xor-output/small-change/add-print2.patch
./Creator-xor bin-output/reference.bin bin-output/small-change/change-string.bin xor-output/small-change/change-string.patch
./Creator-xor bin-output/reference.bin bin-output/small-change/change-value.bin  xor-output/small-change/change-value.patch
./Creator-xor bin-output/reference.bin bin-output/small-change/comment-out-function.bin   xor-output/small-change/comment-out-function.patch
./Creator-xor bin-output/reference.bin bin-output/big-change/add-feature2.bin xor-output/big-change/add-feature2.patch
./Creator-xor bin-output/reference.bin bin-output/big-change/add-feature3.bin xor-output/big-change/add-feature3.patch
./Creator-xor bin-output/reference.bin bin-output/big-change/add-feature.bin xor-output/big-change/add-feature.patch
./Creator-xor bin-output/reference.bin bin-output/big-change/change-commands.bin  xor-output/big-change/change-commands.patch
./Creator-xor bin-output/reference.bin bin-output/big-change/commentout-header.bin   xor-output/big-change/commentout-header.patch
./Creator-xor bin-output/reference.bin bin-output/big-change/many-changees.bin   xor-output/big-change/many-changees.patch
./Creator-xor bin-output/reference.bin bin-output/big-change/Remove-code.bin    xor-output/big-change/Remove-code.patch
./Creator-xor bin-output/reference.bin bin-output/full-change/blink.bin  xor-output/full-change/blink.patch
./Creator-xor bin-output/reference.bin bin-output/full-change/udp-client-rain.bin   xor-output/full-change/udp-client-rain.patch
./Creator-xor bin-output/reference.bin bin-output/full-change/udp-client-temp.bin   xor-output/full-change/udp-client-temp.patch
./Creator-xor bin-output/reference.bin bin-output/full-change/udp-client-video.bin   xor-output/full-change/udp-client-video.patch
./Creator-xor bin-output/reference.bin bin-output/full-change/udp-client-wind.bin   xor-output/full-change/udp-client-wind.patch

