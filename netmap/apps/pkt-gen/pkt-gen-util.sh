#!/bin/bash

IF="ens4f0"
IP_SRC="12.12.12.12" ; MAC_SRC="7c:fe:90:12:22:f0"
IP_DST="12.12.12.13" ; MAC_DST="7c:fe:90:12:23:04"

PKT_SIZE=1442
PORT=2000

sudo ./pkt-gen -i ${IF} -f tx -s ${IP_SRC}:${PORT} -d ${IP_DST}:${PORT} -l ${PKT_SIZE} -S ${MAC_SRC} -D ${MAC_DST}


