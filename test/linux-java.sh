rm -rf outdir/*
mkdir outdir/cp
echo `pwd`/outdir/cp >> `pwd`/outdir/whitelist
echo `pwd`/outdir/cp/demo/Allowed.class >> `pwd`/outdir/whitelist
echo `pwd`/outdir/cp/demo/Works.class >> `pwd`/outdir/whitelist
javac -d outdir/cp -cp outdir/cp test/demo/Allowed.java test/demo/Denied.java
LD_PRELOAD=`pwd`/bin/filterio.so FILTERIO_WHITELIST=`pwd`/outdir/whitelist javac -d outdir/cp -cp outdir/cp test/demo/Works.java test/demo/Fails.java
