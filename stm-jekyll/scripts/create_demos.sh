#!/bin/bash
set -ex

ROOT=$(git rev-parse --show-toplevel)
cd $ROOT
./configure
s/build
s/runtests
demo/run_demo

cp demo/reports/*cr stm-jekyll/_demos
cd stm-jekyll

for filename in _demos/*cr; do
  base=${filename%.*}
  basebase=$(basename "$base")
  php scripts/cr2svg.php --html $base.cr $base.html
  if [ ! -e $base.md ]; then
    echo "---" >> $base.md
    echo "name: $basebase" >> $base.md
    echo "cr: $basebase.cr" >> $base.md
    echo "---" >> $base.md
    echo >> $base.md
  fi
done
