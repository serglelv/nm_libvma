
netmap_vma Installation Instructions
====================================

Install and build the LIBVMA https://github.com/Mellanox/libvma
Install the netmap framework https://github.com/luigirizzo/netmap
Install the biTStream https://code.videolan.org/videolan/bitstream.git

The following parameters in the nm_vma.mk should be modified accordingly:
VMA_SRC_DIR, NETMAP_SRC_DIR, BITSTREAM_DIR

