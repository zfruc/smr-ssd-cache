#!/bin/bash
blocksize_kb=4
counter=16
min=1
SSD_CACHE_BLKSIZE=100000
CACHE_INTERVAL=5000
while (($counter>=min))
do
	./smr-ssd-cache $SSD_CACHE_BLKSIZE > ../$(($SSD_CACHE_BLKSIZE/1000*$blocksize_kb))M
	SSD_CACHE_BLKSIZE=$(($SSD_CACHE_BLKSIZE-$CACHE_INTERVAL))
	counter=$(($counter-1))
	rm /dev/shm/*
done

