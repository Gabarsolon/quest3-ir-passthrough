#!/system/bin/sh
mkdir -p /data/local/tmp/fdump
cd /data/local/tmp
for fd in 262 268 274 280 286 292 298 304 310 316 322 328 334 340 346 352 358 364 370 376 382 388 394 400 406 412; do
  ./dumpdbg2 852 $fd /data/local/tmp/fdump/f_$fd.raw 327680
done
echo done
