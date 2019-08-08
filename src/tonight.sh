#!/bin/bash

./exps/fio_benchmarks/paperexps/smr-pb-forceclean.sh /mnt/smr/smr-rawdisk big

./smr-ssd-cache 0 11 0 0 8000000 8000000 30 PAUL -1 > /home/fei/devel/logs/long-rw-real-paul-bug123.log
