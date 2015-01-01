#/bin/bash
bash -c "sudo losetup /dev/loop0 $(dirname $0)/disk.img"
bash -c "sudo partx -a /dev/loop0 2> /dev/null"
bash -c "sudo mount /dev/loop0p1 $(dirname $0)/mnt"
