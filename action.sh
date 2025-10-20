#!/system/bin/sh
[ ! "$(whoami)" = "root" ] && echo " » 请授予root权限！" && exit 1
######
home="${0%/*}"
exec 2>>/dev/null
######
sh "$home/service.sh"
