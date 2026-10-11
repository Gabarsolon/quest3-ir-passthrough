#!/system/bin/sh
P=852
for i in 1 2 3 4 5 6; do
  L=$(od -An -tu8 /data/local/tmp/ir.raw.rgba.seq 2>/dev/null)
  R=$(od -An -tu8 /data/local/tmp/ir_r.raw.rgba.seq 2>/dev/null)
  echo "t$i L=$L R=$R"
  sleep 3
done
