#/bin/bash
bash -c "sudo umount /dev/loop0p1"
bash -c "sudo losetup -d /dev/loop0"
