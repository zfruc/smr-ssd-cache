#!/bin/bash

# checkout branch *MOST
git checkout branch-MOST-test

LOG_PATH=/home/fei/devel/logs
LOG_FN=most_real_wo_x10_long.log
PATH_MAIN=/home/fei/devel/smr-ssd-cache_multi-user/src
PATH_CLEAN=${PATH_MAIN}/exps/fio_benchmarks/paperexps

make clean ; make

bash ${PATH_MAIN}/smr-ssd-cache 0 11 1 0 8000000 8000000 1 PAUL -1 > ${LOG_PATH}/${LOG_FN}
bach ${PATH_CLEAN}/smr-pb-forceclean.sh /mnt/smr/smr-rawdisk big 


# checkout branch *master
LOG_FN2=paul_real_wo_x10_long.log

git checkout master 
git pull

make clean ; make

bash ${PATH_MAIN}/smr-ssd-cache 0 11 1 0 8000000 8000000 30 PAUL -1 > ${LOG_PATH}/${LOG_FN2}
bach ${PATH_CLEAN}/smr-pb-forceclean.sh /mnt/smr/smr-rawdisk big



