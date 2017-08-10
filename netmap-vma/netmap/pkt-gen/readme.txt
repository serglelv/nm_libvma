1. sudo insmod netmap.ko
2. Configure the pkt-gen-util.sh script
   2.1 Set your interface and IP_SRC and MAC_DST

   IF="eth0"
   IP_SRC="XX.XX.XX.XX" ; MAC_SRC="XX:XX:XX:XX:XX:XX"
   IP_DST="XX.XX.XX.XX" ; MAC_DST="XX:XX:XX:XX:XX:XX"
