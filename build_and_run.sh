

ROOT=$PWD

$ROOT/build.sh

STATUS=$?

if test $STATUS -eq 0 
then
    echo 'Success'
    $ROOT/build/base_test.exe
else
   echo 'Failure'
fi