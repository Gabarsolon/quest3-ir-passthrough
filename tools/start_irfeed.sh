#!/system/bin/sh
cd /data/local/tmp
if [ -f ir_feed.pid ]; then kill "$(cat ir_feed.pid)" 2>/dev/null; fi
pkill -f start_ir_feed 2>/dev/null
pkill dumpdbg 2>/dev/null
sleep 1
rm -f /data/local/tmp/ir.raw /data/local/tmp/irfeed.log
setsid sh /data/local/tmp/run_irfeed.sh > /data/local/tmp/irfeed.log 2>&1 < /dev/null &
echo "launched $!"
