rm -rf outdir/*
mkdir outdir/cp
echo `pwd`/outdir/cp >> `pwd`/outdir/whitelist
echo `pwd`/outdir/cp/demo/Whitelisted.class >> `pwd`/outdir/whitelist
scalac -d outdir/cp -cp outdir/cp test/demo/Traits.scala
DYLD_INSERT_LIBRARIES=`pwd`/bin/filterio.dylib FILTERIO_WHITELIST=`pwd`/outdir/whitelist scalac -d outdir/cp -cp outdir/cp test/demo/Demo.scala
