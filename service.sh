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
DisableApp=$(grep ^'冻结=' "$home_dir/config.prop" | cut -f2 -d '=')
CheckApp=$(grep ^'检测=' "$home_dir/config.prop" | cut -f2 -d '=')
if [ -n "$DisableApp" ]; then
    if [ -n "$CheckApp" ]; then
        killall AutoDisableServer 2>/dev/null; nohup setsid AutoDisableServer >>/dev/null &
        echo -n "*/30 * * * * killall AutoDisableServer 2>/dev/null; nohup setsid AutoDisableServer >>/dev/null &" > "$home_dir/CRON/root"
        "$bin_dir/busybox" crond -c "$home_dir/CRON" &
        echo "Server is Run" > "$home_dir/LOG.log"
    else
        echo "Error: “检测=” 为空" > "$home_dir/LOG.log"
    fi
else
    echo "Error: “冻结=” 为空" > "$home_dir/LOG.log"
fi

exit 0
