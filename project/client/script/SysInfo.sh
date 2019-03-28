#!/bin/bash
function Usage() {
	echo "Usage: $0"
}

if [[ $# -ge 1 ]]; then
	Usage
	exit 1
fi




NowTime=`date +"%Y-%m-%d__%H:%M:%S"`

HostName=`hostname`

OsType=`cat /etc/issue.net | tr -s  " " "_"`

KernelVersion=`uname -r`

LoadAvg=`cut -d " " -f 1-3 /proc/loadavg`

UpTime=`uptime -p | tr  -s " " "_"`

eval $(free -m | head -n 2 | tail -n 1 | awk '{printf("MemSum=%d;MemUsed=%d",$2,$3)}')

MemUsedPerc=$[ $MemUsed*100/$MemSum ]

MemWarningLevel="normal"
if [[ $MemUsedPerc -gt 90 ]]; then
	MemWarningLevel="warning"
elif [[ $MemUsedPerc -gt 80 ]]; then
	MemWarningLevel="note"
fi

eval $(df -T -m -x tmpfs -x devtmpfs | tail -n +2 | \
	awk  '{printf("paramount["NR"]=%d;parused["NR"]=%d;",$3,$5,$7,$6)} \
	END {printf("parnum=%d\n",NR)}')

for (( i = 1; i <= $parnum; i++ )); do
	DiskSum=$[ $DiskSum + ${paramount[$i]} ]
	DiskUsedSum=$[ $DiskUsedSum + ${parused[$i]} ]
done

DiskUsedPercSum=$[ ($DiskUsedSum*100/$DiskSum) ]

DiskWarningLevel="normal"

if [[ $DiskUsedPercSum -gt 90 ]]; then
	DiskWarningLevel="warning"
elif [[ $DiskUsedPercSum -gt 80 ]]; then
	DiskWarningLevel="note"
fi

CpuTemp=`cat /sys/class/thermal/thermal_zone0/temp`
CpuTemp=`echo "scale=2;$CpuTemp/1000" | bc`

CpuWarnLevel="normal"

if [[ `echo $CpuTemp '>=' 70 | bc -l` = 1 ]]; then
	CpuWarnLevel="warning"
elif [[ `echo $CpuTemp '>=' 50 | bc -l` = 1 ]]; then
	CpuWarnLevel="note"
fi

echo "$NowTime $HostName $OsType $KernelVersion $UpTime $LoadAvg \
$DiskSum ${DiskUsedPercSum}% $MemSum ${MemUsedPerc}% $CpuTemp $DiskWarningLevel \
$MemWarningLevel $CpuWarnLevel" #>> $SysInfoLog
