WORKDIR=$(dirname "$0")
rm $WORKDIR/.oram_state
$WORKDIR/clear_fdb
$WORKDIR/server &
PID=$!
$WORKDIR/test_fdb
kill $PID
