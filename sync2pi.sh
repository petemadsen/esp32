#!/bin/bash

FROM=(khaus xmastree)

REMOTE_DIR=/home/pi/awShutters/User/rpi.py/src/ota
DEST=pi1:$REMOTE_DIR

for project in "${FROM[@]}"
do
	echo $project
	FILE=$project/build/$project.bin
	rsync -avzP $FILE $DEST
done

echo ""
echo "Remote files"
echo "============"
ssh pi1 "ls -lh $REMOTE_DIR"
