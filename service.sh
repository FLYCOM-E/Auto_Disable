#!/system/bin/sh
# by FlyCome 
[ ! "$(whoami)" = "root" ] && echo " » 请授予root权限！" && exit 1
######
home_dir="${0%/*}"
######
set=0
while [ "$(getprop sys.boot_completed)" != "1" ]; do
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
else
    echo "Check Bin Directory Error" > "$home_dir/LOG.log"
    exit 1
fi
######
chmod +x "$home_dir/AutoDisableServer"
######
if ! pgrep -f "AutoDisableServer"; then
    nohup setsid "$home_dir/AutoDisableServer" "$home_dir" >>/dev/null 2>&1 &
    if pgrep -f "AutoDisableServer"; then
        echo "AutoDisableServer is Run" > "$home_dir/LOG.log"
        echo -n "*/30 * * * * pgrep -f \"AutoDisableServer\" || nohup setsid \"$home_dir/AutoDisableServer\" \"$home_dir\" >>/dev/null 2>&1" > "$home_dir/CRON/root"
        "$bin_dir/busybox" crond -c "$home_dir/CRON" &
    else
        echo "AutoDisableServer Run Error" > "$home_dir/LOG.log"
    fi
fi
