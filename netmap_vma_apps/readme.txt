1. Build netmap_vma_ex1
   1.1 Set your path VMA_DIR = to the VMA sources in the makefile
   1.2 $make
   1.3 Crate interface configuration file
      please see sample file ens4f0.txt
      ens4f0.txt -> your_interface_name.txt
        please set your interface ip address
	XX.XX.XX.XX  2000  1
   
2. Set your path in the netmap_vma_ex1.sh
   PRELOAD="LD_PRELOAD=/your-path/libvma/src/vma/.libs/libvma.so"

3. $./netmap_vma_ex1.sh

