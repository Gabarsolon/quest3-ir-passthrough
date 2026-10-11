#!/system/bin/sh
cd /data/local/tmp
rm -f /data/local/tmp/ir_r.raw /data/local/tmp/ir_r.raw.rgba /data/local/tmp/irfeed_r.log
setsid sh /data/local/tmp/run_irfeed_r.sh > /data/local/tmp/irfeed_r.log 2>&1 < /dev/null &
echo "launched $!"
