1. Install netmap framework  https://github.com/luigirizzo/netmap

2. Configure and build the framework  
   
3. sudo insmod netmap.ko 

4. Configure the pkt-gen-util.sh script
   4.1 Set your interface and IP_SRC and MAC_DST

   IF="ens4f0"
   IP_SRC="12.12.12.12" ; MAC_SRC="7c:fe:90:12:22:f0"
   IP_DST="12.12.12.13" ; MAC_DST="7c:fe:90:12:23:04"
