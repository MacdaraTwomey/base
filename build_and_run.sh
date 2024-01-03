

ROOT=$PWD

$ROOT/build.sh win

STATUS=$?

if test $STATUS -eq 0 
then
    echo 'Success'
    $ROOT/build/test.exe
else
   echo 'Failure'
fi