#!/system/bin/sh
cd /data/local/tmp
P=852
if [ -f /data/local/tmp/ir_fds.txt ]; then
  LIST=$(cat /data/local/tmp/ir_fds.txt)
else
  LIST=""
  for f in /proc/$P/fd/*; do
    t=$(readlink "$f" 2>/dev/null)
    case "$t" in *dmabuf*) ;; *) continue;; esac
    s=$(stat -c %s "$f" 2>/dev/null)
    [ "$s" = "327680" ] && LIST="$LIST ${f##*/}"
  done
fi
echo "fds:$LIST"
exec ./irfeed $P /data/local/tmp/ir.raw $LIST
