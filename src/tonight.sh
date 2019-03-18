#!/bin/bash

./exps/fio_benchmarks/paperexps/smr-pb-forceclean.sh /mnt/smr/smr-rawdisk big

./smr-ssd-cache 0 11 1 0 8000000 8000000 30 OLDPORE -1 > /home/fei/devel/logs/long-wo-real-oldpore-3mil-new.log
