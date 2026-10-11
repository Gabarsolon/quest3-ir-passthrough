#!/system/bin/sh
cd /data/local/tmp
P=852
LIST=$(cat /data/local/tmp/ir_fds_r.txt)
echo "fds:$LIST"
while true; do
  ./irfeed $P /data/local/tmp/ir_r.raw $LIST
  echo "irfeed_r exited -> restart"
  sleep 0.3
done
