#!/bin/bash
function Usage() {
	echo "Usage: $0"
}

if [[ $# -ge 1 ]]; then
	Usage
	exit 1
fi



eval $(ps -aux --sort=-%cpu -h | awk  -v num=0 \
	'{if($3 < 50) {exit} else {num++;printf("cpuproid["num"]=%d;",$2);} } END {printf("cpunum=%d;",num)}')

eval $(ps -aux --sort=-%mem -h | awk  -v num=0 \
	'{if($4 < 50) {exit} else {num++;printf("memproid["num"]=%d;",$2);} } END {printf("memnum=%d;",num)}')

if [[ $cpunum -gt 0 || $memnum -gt 0 ]]; then
	sleep 10
	else
	exit 0	
fi

NowTime=`date +"%Y-%m-%d__%H:%M:%S"`

if [[ $cpunum -gt 0 ]]; then
	count=0
	for i in ${cpuproid[*]}; do
		count=$[ $count + 1 ]
		eval $(ps -aux -q $i -h  | awk  -v num=$count \
			'{if($3 < 50) {exit} else {printf("cpuproname["num"]=%.40s;\
			cpuproid["num"]=%d;cpuprouser["num"]=%s;cpuprocpu["num"]=%2.2f;\
			cpupromem["num"]=%2.2f;",$11,$2,$1,$3,$4);} }\
			END {printf("cpuchecknum=%d;",num)}')
	done
fi

if [[ $memnum -gt 0 ]]; then
	count=0
	for i in ${memproid[*]}; do
		count=$[ $count + 1 ]
		eval $(ps -aux -q $i -h  | awk  -v num=$count \
			'{if($4 < 50) {exit} else {printf("memproname["num"]=%.40s;memproid["num"]=%d;memprouser["num"]=%s;memprocpu["num"]=%s;mempromem["num"]=%s;"\
			,$11,$2,$1,$3,$4);} } END {printf("memchecknum=%d;",num)}')

	done

fi


if [[ $cpuchecknum -gt 0 ]]; then
	for (( i = 1; i <= $cpuchecknum; i++ )); do
		echo "$NowTime ${cpuproname[$i]} ${cpuproid[$i]} ${cpuprouser[$i]} ${cpuprocpu[$i]}% ${cpupromem[$i]}%"
	done
fi

if [[ $memchecknum -gt 0 ]]; then
	for (( i = 1; i <= $memchecknum; i++ )); do
		echo "$NowTime ${memproname[$i]} ${memproid[$i]} ${memprouser[$i]} ${memprocpu[$i]}% ${mempromem[$i]}%"
	done
fi
