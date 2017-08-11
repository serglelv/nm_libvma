
The script pkt-gen-util.sh requires some small modifications.

* $./pkt-gen-util.sh -t
  The script pkt-gen-util.sh (tx mode) should be configured to 
  use the netmap API. You need modify the following parameters: 
  IF_TX, SRC_IP, SRC_MAC, DST_IP and DST_MAC

* $./pkt-gen-util.sh -r
  The script pkt-gen-vma (rx mode) should be configured to use 
  the netmap_vma API. You need modify the following parameters: 
  PRELOAD and IF_RX
  