#!/bin/bash

PKT_SIZE=1442
PORT=2000

#########################
# pkt-gen netmap TX 
#########################
IF_TX="ens4f0"
SRC_IP="12.12.12.12" ; SRC_MAC="7c:fe:90:12:22:f0"
DST_IP="12.12.12.13" ; DST_MAC="7c:fe:90:12:23:04"

TX_COUNT=""
TX_RATE=""

pkt_tx()
{
	./pkt-gen -i netmap:${IF_TX} -f tx "${TX_RATE}" "${TX_COUNT}" -l ${PKT_SIZE} -s ${SRC_IP}:${PORT} -d ${DST_IP}:${PORT} -S ${SRC_MAC} -D ${DST_MAC}
}

#########################
# pkt-gen netmap_vma RX 
#########################
IF_RX="ens4f0"
PRELOAD="LD_PRELOAD=/path_to_libvma/src/vma/.libs/libvma.so"

LD_PATH="LD_LIBRARY_PATH=../../netmap-vma"

pkt_rx()
{
	sh -c "${PRELOAD} ${LD_PATH} ./pkt-gen-vma -i netmap:${IF_RX}-1/R:${PORT} -f rx"
}

usage()
{
cat << eOm
	usage:$0 [-t|--tx N] [--count N] [--rate N] [-r]
eOm
	exit 0
}

[ $# -eq 0 ] && usage

OPTS=$(getopt -o hrt -l tx:,count:,rate: -- "$@")
[[ $? -ne 0 ]] && usage
eval set -- "${OPTS}"

OPTR=""
while true ; do
	case "$1" in
		-r)
			OPTR="pkt_rx"
			shift 1
			;;
		-t)
			OPTR="pkt_tx"
			shift 1
			;;
		--count)
			[[ "$2" =~ ^-?[0-9]+$ ]] && TX_COUNT="-n $2"
			shift 2
			;;
		--rate)
			[[ "$2" =~ ^-?[0-9]+$ ]] && TX_RATE="-R $2"
			shift 2
			;;
		--)
			shift
			break
			;;
		*)
			usage
			;;
	esac
done

[ "${OPTR}" != "" ] && ${OPTR} "${OPTS}"

# </pkt-gen-util.sh>
