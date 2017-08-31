#! /bin/bash

## 功能:修改单架构.a文件中所有.o的符号表

function redefine_one()
{
LIB_ORI=$1
LIB_DEST=$2
REDEFINE=$3

LIBINFO=`lipo -info $LIB_ORI`
if [[ ! $LIBINFO =~ 'is not a fat file' ]] 
then
echo "failed. can't deal fat file."
return 1
fi

WORK_DIR=TMP_`date +%s`
rm -rf $WORK_DIR
mkdir $WORK_DIR
cp $LIB_ORI $WORK_DIR
cp redefine_sym_mac $WORK_DIR
cd $WORK_DIR
chmod +x redefine_sym_mac
ar -x ../$LIB_ORI
rm -rf *.hack
OBJS=`ls *.o`
for OBJ in $OBJS
do
./redefine_sym_mac $OBJ ${OBJ}.hack $REDEFINE
if [ $? -ne 0 ]
then
    cd ..
    rm -rf $WORK_DIR
    return 1
fi

if [ -f ${OBJ}.hack ]
then
rm -rf $OBJ
mv ${OBJ}.hack $OBJ
fi
done

rm -rf ../$LIB_DEST
#ar cru $LIB_DEST *.o
#ranlib $LIB_DEST
libtool -static -o ../$LIB_DEST *o
cd ..
rm -rf $WORK_DIR
return 0
}

redefine_one $1 $2 "$3"
if [ $? -ne 0 ]
then
exit 1
fi
exit 0
