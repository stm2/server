#!/bin/bash
set -ex

ROOT=$(git rev-parse --show-toplevel)
cd $ROOT
#./configure
#s/build
#s/runtests
demo/run_demo

cp demo/reports/*md stm-jekyll/_data/crs
cp demo/reports/*cr stm-jekyll/_data/crs
cd stm-jekyll

for filename in _data/crs/*md; do
  base=${filename%.*}
  basebase=$(basename "$base")
  php scripts/cr2svg.php --annotate $base.md _demos/$basebase.md
done

for filename in _data/crs/*cr; do
  base=${filename%.*}
  basebase=$(basename "$base")
  if [ -e $base.md ]; then
    php scripts/cr2svg.php --md $base.md $base.cr _demos/$basebase.md
  else
    php scripts/cr2svg.php --html $base.cr _demos/$basebase.md
  fi
done
