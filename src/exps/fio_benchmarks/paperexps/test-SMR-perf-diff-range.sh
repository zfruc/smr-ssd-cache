#!/bin/bash
#每次测试前需要确保SMR的Persistent Buffer(PB)是否已经被完全清空，若不是，请顺序写遍整个磁盘。

echo "256M..."
fio --filename=/dev/sdc --filesize=256M ./rdm_w_nobuf.fio > R-256M.log
dd if=/dev/zero of=/dev/sdc bs=4K count=64K

echo "512M..."
fio --filename=/dev/sdc --filesize=512M ./rdm_w_nobuf.fio > R-512M.log
dd if=/dev/zero of=/dev/sdc bs=4K count=128K

echo "1G..."
fio --filename=/dev/sdc --filesize=1G ./rdm_w_nobuf.fio > R-1G.log
dd if=/dev/zero of=/dev/sdc bs=4K count=256K

echo "2G..."
fio --filename=/dev/sdc --filesize=2G ./rdm_w_nobuf.fio > R-2G.log
dd if=/dev/zero of=/dev/sdc bs=4K count=512K

echo "4G..."
fio --filename=/dev/sdc --filesize=4G ./rdm_w_nobuf.fio > R-4G.log
dd if=/dev/zero of=/dev/sdc bs=4K count=1M

echo "8G..."
fio --filename=/dev/sdc --filesize=8G ./rdm_w_nobuf.fio > R-8G.log
dd if=/dev/zero of=/dev/sdc bs=4K count=2M

echo "16G..."
fio --filename=/dev/sdc --filesize=16G ./rdm_w_nobuf.fio > R-16G.log
dd if=/dev/zero of=/dev/sdc bs=4K count=4M

echo "32G..."
fio --filename=/dev/sdc --filesize=32G ./rdm_w_nobuf.fio > R-32G.log
dd if=/dev/zero of=/dev/sdc bs=4K count=8M

echo "64G..."
fio --filename=/dev/sdc --filesize=64G ./rdm_w_nobuf.fio > R-64G.log
dd if=/dev/zero of=/dev/sdc bs=4K count=16M

echo "128G..."
fio --filename=/dev/sdc --filesize=128G ./rdm_w_nobuf.fio > R-128G.log
dd if=/dev/zero of=/dev/sdc bs=4K count=32M

echo "256G..."
fio --filename=/dev/sdc --filesize=256G ./rdm_w_nobuf.fio > R-256G.log
dd if=/dev/zero of=/dev/sdc bs=4K count=64M

echo "512G..."
fio --filename=/dev/sdc --filesize=512G ./rdm_w_nobuf.fio > R-512G.log
dd if=/dev/zero of=/dev/sdc bs=4K count=128M

echo "1T..."
fio --filename=/dev/sdc --filesize=1T ./rdm_w_nobuf.fio > R-1T.log
dd if=/dev/zero of=/dev/sdc bs=4K count=256M

echo "2T..."
fio --filename=/dev/sdc --filesize=2T ./rdm_w_nobuf.fio > R-2T.log
dd if=/dev/zero of=/dev/sdc bs=4K count=512M

echo "4T..."
fio --filename=/dev/sdc --filesize=4T ./rdm_w_nobuf.fio > R-4T.log
dd if=/dev/zero of=/dev/sdc bs=4K count=1G

echo "finish!"
