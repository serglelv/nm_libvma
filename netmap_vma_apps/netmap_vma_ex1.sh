#!/bin/bash

PRELOAD="LD_PRELOAD=/your-path/libvma/src/vma/.libs/libvma.so"
LD_LIB_PATH="LD_LIBARAY_PATH=../netmap_vma:${LD_LIBRARY_PATH}"

sudo sh -c "${PRELOAD} ${LD_LIB_PATH} ./netmap_vma_ex1 "

