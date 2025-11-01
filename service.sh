#!/system/bin/sh
# by FlyCome 
[ ! "$(whoami)" = "root" ] && echo " » 请授予root权限！" && exit 1
######
home_dir="${0%/*}"
exec 2>>/dev/null
######
set=0
while true; do
    [ "$(getprop sys.boot_completed)" = "1" ] || [ -d "/storage/emulated/0/" ] && break
    [ "$set" = 60 ] && touch "$home_dir/disable" && exit 1
    set=$((set + 1))
    sleep 10
done
######
if [ -d "/data/adb/magisk" ]; then
    bin_dir="/data/adb/magisk"
elif [ -d "/data/adb/ap" ]; then
    bin_dir="/data/adb/ap/bin"
elif [ -d "/data/adb/ksu" ]; then
    bin_dir="/data/adb/ksu/bin"
fi
######
if [ -z "$(cat CheckList.conf)" ]; then
    echo " » CheckList.conf 为空！" > "$home_dir/LOG.log"
    exit 1
elif [ -z "$(cat DisableList.conf)" ]; then
    echo " » DisableList.conf 为空！" > "$home_dir/LOG.log"
    exit 1
fi
######
chmod +x "$home_dir/AutoDisableServer"
######
if ! ps -A | grep "S AutoDisableServer"; then
    nohup setsid "$home_dir/AutoDisableServer $home_dir" >>/dev/null &
    if ps -A | grep "S AutoDisableServer"; then
        echo "AutoDisableServer is Run" > "$home_dir/LOG.log"
        echo -n "*/30 * * * * ps -A | grep AutoDisableServer || nohup setsid $home_dir/AutoDisableServer $home_dir >>/dev/null &" > "$home_dir/CRON/root"
        "$bin_dir/busybox" crond -c "$home_dir/CRON" &
    else
        echo "AutoDisableServer Run Error" > "$home_dir/LOG.log"
    fi
fi

