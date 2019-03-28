#!/bin/bash
function Usage() {
	echo "Usage: $0"
}

if [[ $# -ge 1 ]]; then
	Usage
	exit 1
fi



export PROCPS_USERLEN=16
NowTime=`date +"%Y-%m-%d__%H:%M:%S"`

eval $(awk -F: -v sum=0 '{if($3 >=1000 && $3 != 65534){sum+=1;printf("User["sum"]=%s;",$1)} } \
	END {printf("UserNum=%d;",sum)}' /etc/passwd)

MostActiveUser=`last -w |cut -d " "  -f 1 |grep -v root | grep -v wtmp | grep -v reboot |\
	grep "[a-zA-Z]" | sort | uniq -c | sort -k 1 -n -r | awk -v num=3\
	 '{if(num >0){printf(",%s",$2);num--}}'|cut -c 2-`

eval $(awk -F : '{if($3 == 1000) printf("UserWithRoot=%s",$1)}' /etc/passwd)
userofroot=`grep sudo /etc/group | cut -d : -f 4| tr  ',' ' '`
for i in $userofroot;do
    if [[ $i = $UserWithRoot ]] ;then
        continue
    fi
    UserWithRoot="${UserWithRoot},$i"
done

if [[  -r /home/sudoers  ]]; then
	for (( i = 1; i < $UserNum; i++ )); do
		grep -q "^${User[$i]}" /home/sudoers
		if [[ $? -eq 0 ]]; then
			UserWithRoot="$UserWithRoot,${User[$i]}"
		fi
	done
	else
		UserWithRoot="${UserWithRoot},Failed_with_perm"
fi
UserLogedIn=`w -h | awk '{printf(",%s_%s_%s",$1,$3,$2)}' | cut -c 2-`

echo "$NowTime $UserNum [${MostActiveUser}] [${UserWithRoot}] [${UserLogedIn}]" ##>> $UsersLog










