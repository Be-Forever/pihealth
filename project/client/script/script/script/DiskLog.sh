#!/bin/bash
function Usage() {
	echo "Usage: $0"
}

if [[ $# -ge 1 ]]; then
	Usage
	exit 1
fi

source ./PiHealth.conf >> /dev/zero

if [[ $? -ne 0 ]]; then
	source ./script/PiHealth.conf
	sleep 60
	if [[ $? -ne 0 ]]; then
		exit
	fi
fi

NowTime=`date +"%Y-%m-%d__%H:%M:%S"`

eval $(df -T -m -x tmpfs -x devtmpfs | tail -n +2 | grep "dev" | \
	awk -v DiskSum=0 -v LeftSum=0 '{printf("paramount["NR"]=%d;parleft["NR"]=%d;parname["NR"]=%s;\
	usedperc["NR"]=%s;",$3,$5,$7,$6);DiskSum=DiskSum+$3;LeftSum=LeftSum+$5} \
	END {printf("parnum=%d;DiskSum=%d;LeftSum=%d",NR,DiskSum,LeftSum)}')

for (( i = 1; i <= $parnum; i++ )); do
	echo "$NowTime 1 ${parname[$i]} ${paramount[$i]} ${parleft[$i]}\
	 ${usedperc[$i]}" #>>$DiskLog
done
UsedPercSum=$[ (100-$LeftSum*100/$DiskSum) ]
echo "$NowTime 0 disk $DiskSum $LeftSum ${UsedPercSum}%" #>>$DiskLog
echo " "
























