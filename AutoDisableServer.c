// By FlyCome 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <android/log.h>

#define GET_TOPAPP "dumpsys window | grep mCurrentFocus | head -n 1 | cut -f 1 -d '/' | cut -f 5 -d ' ' | cut -f 1 -d ' '"
#define LOGPRINT __android_log_print
#define MAX_PATH 4096
#define MAX_PACKAGE 256
#define MAX_CONFIG 10
#define WAITTIME 10

static int service(int mode, char * config_dir, char * package);

int main(int argc, char * argv[])
{
    if (getuid() != 0 && getuid() != 2000) //检查当前用户权限
    {
        printf(" » Please Use Root or Shell Run\n");
        return 0;
    }
    if (argc < 2) //检查参数数量
    {
        printf(" » 未传入配置路径！\n");
        return 1;
    }
    else if (access(argv[1], F_OK) != 0) //检查路径能否访问
    {
        printf(" » 配置路径不可访问或不存在！\n");
        return 1;
    }
    
    // 遍历配置目录逐一读取保存声明包名
    int file_count = 0;
    char package_list[MAX_CONFIG][MAX_PACKAGE] = {0};
    struct dirent * entry;
    DIR * config_dir_dp = opendir(argv[1]);
    if (config_dir_dp == NULL)
    {
        printf(" » 配置目录打开失败！\n");
        return 1;
    }
    while ((entry = readdir(config_dir_dp)))
    {
        if (strcmp(entry -> d_name, ".") == 0 ||
           strcmp(entry -> d_name, "..") == 0)
        {
            continue;
        }
        
        char config_path[MAX_PATH] = "", config_line[MAX_PACKAGE] = "";
        snprintf(config_path, sizeof(config_path), "%s/%s", argv[1], entry -> d_name);
        FILE * config_path_fp = fopen(config_path, "r");
        if (config_path_fp == NULL)
        {
            printf(" » %s 配置打开失败，自动跳过！\n", entry -> d_name);
            continue;
        }
        if (fgets(config_line, sizeof(config_line), config_path_fp))
        {
            /*
            配置声明：@<包名>
            */
            config_line[strcspn(config_line, "\n")] = 0;
            char * config_line_p = config_line;
            while (isspace(* config_line_p)) config_line_p++;
            
            if (* config_line_p != '@')
            {
                printf(" » %s 配置声明错误，自动跳过！\n", entry -> d_name);
                continue;
            }
            if (file_count <= MAX_CONFIG)
            {
                snprintf(package_list[file_count], sizeof(package_list[file_count]), "%s", config_line + 1);
                printf(" » 已加载配置 %s，包名 %s\n", entry -> d_name, config_line + 1);
                file_count++; // 已读取配置+1
            }
            else
            {
                printf(" » 加载配置数量超过限制，已加载 %d 个配置，跳过其它配置！\n", file_count);
                break;
            }
        }
        else
        {
            printf(" » %s 配置加载失败！已跳过\n", entry -> d_name);
        }
        fclose(config_path_fp);
    }
    closedir(config_dir_dp);
    
    if (file_count == 0) // 未读取任何配置
    {
        printf(" » 配置目录为空！未读取任何配置\n");
        return 1;
    }
    
    pid_t PID = fork();
    if (PID == -1)
    {
        printf(" » 进程启动失败！\n");
        return 1;
    }
    else if (PID != 0)
    {
        exit(0);
    }
    setsid();
    int std = open("/dev/null", O_RDWR);
    dup2(std, STDIN_FILENO);
    dup2(std, STDOUT_FILENO);
    dup2(std, STDERR_FILENO);
    close(std);
    
    //定义循环所需变量
    int mode = 0, max_get_topapp_error = 0;
    char old_top_app[MAX_PACKAGE] = "";
    //Start the cycle
    for ( ; ; )
    {
        //Get获取当前前台
        char top_app[MAX_PACKAGE] = "";
        FILE * top_app_fp = popen(GET_TOPAPP, "r");
        if (top_app_fp == NULL)
        {
            /* 以下 if 块是检查获取前台 App 失败次数决定是否退出
            成功获取 App 后次数记录会重置 */
            sleep(5);
            max_get_topapp_error++;
            if (max_get_topapp_error == 5)
            {
                sleep (5);
            }
            else if (max_get_topapp_error == 20)
            {
                LOGPRINT(ANDROID_LOG_ERROR, "AutoDisable", "Get TopApp Failed! Timeout...\n");
                break;
            }
            LOGPRINT(ANDROID_LOG_WARN, "AutoDisable", "Get TopApp Failed! Again...\n");
            continue;
        }
        fgets(top_app, sizeof(top_app), top_app_fp);
        pclose(top_app_fp);
        top_app[strcspn(top_app, "\n")] = 0;
        max_get_topapp_error = 0; // 重置失败次数
        
        //检查屏幕状态
        if (strcmp(top_app, "") == 0 ||
           strstr(top_app, "NotificationShade") ||
           strstr(top_app, "StatusBar") ||
           strstr(top_app, "ActionsDialog"))
        {
            LOGPRINT(ANDROID_LOG_INFO, "AutoDisable", "Top is SystemUI. Wait\n");
            sleep(WAITTIME);
            continue;
        }
        
        //判断当前状态，置 1 ：当前冻结列表App已冻结，待解冻
        if (mode == 1)
        {
            // 这里检查当前前台是否未变化
            if (strcmp(top_app, old_top_app) == 0)
            {
                LOGPRINT(ANDROID_LOG_INFO, "AutoDisable", "App Not Cycle. Wait\n");
                sleep(WAITTIME);
                continue;
            }
            else
            {
                mode = 2; // 置 2 准备解冻 App 列表
            }
        }
        
        // 读取检测列表并检测当前前台 App 是否在该列表
        int i = 0;
        while (i < file_count)
        {
            if (strcmp(package_list[i], top_app) == 0)
            {
                if (mode == 2)
                {
                    service(2, argv[1], old_top_app);
                }
                /* 如果当前前台是被监测 App 则置 mode=1 并保留包名以检测变化
                这里不会影响前面置2的行为，因为如果前台仍然是被监测App就没必要解冻App列表 */
                mode = 1;
                snprintf(old_top_app, sizeof(old_top_app), "%s", package_list[i]); // 保留包名（对应配置）
                break;
            }
            i++;
        }
        
        if (mode == 1)
        {
            int end = service(mode, argv[1], package_list[i]);
            if (end != 0)
            {
                LOGPRINT(ANDROID_LOG_WARN, "AutoDisable", "Error Code %d\n", end);
            }
        }
        else if (mode == 2)
        {
            int end = service(mode, argv[1], old_top_app);
            if (end == 0)
            {
                mode = 0; //解冻后重置此值
            }
            else
            {
                LOGPRINT(ANDROID_LOG_WARN, "AutoDisable", "Error Code %d\n", end);
            }
        }
        
        LOGPRINT(ANDROID_LOG_INFO, "AutoDisable", "Cycle Wait.\n");
        sleep(WAITTIME);
    }
    return 0;
}

/*
mode 为 1 则读取冻结 App 列表并逐一冻结
mode 为 2 则读取冻结 App 列表并逐一解冻
返回 0 成功 1 错误
*/
static int service(int mode, char * config_dir, char * package)
{    
    // 定义模式对应pm参数，同时检查是否需要操作
    char arg[64] = "";
    if (mode == 1)
    {
        snprintf(arg, sizeof(arg), "disable-user");
    }
    else if (mode == 2)
    {
        snprintf(arg, sizeof(arg), "enable");
    }
    if (access(config_dir, F_OK) != 0)
    {
        return 1;
    }
    
    int end = 0;
    struct dirent * entry;
    DIR * config_dir_dp = opendir(config_dir);
    if (config_dir_dp == NULL)
    {
        return 1;
    }
    while ((entry = readdir(config_dir_dp)))
    {
        if (strcmp(entry -> d_name, ".") == 0 ||
           strcmp(entry -> d_name, "..") == 0)
        {
            continue;
        }
        if (end == 1)
        {
            break;
        }
        
        int count = 0;
        char config_path[MAX_PATH] = "", config_line[MAX_PACKAGE] = "";
        snprintf(config_path, sizeof(config_path), "%s/%s", config_dir, entry -> d_name);
        FILE * config_path_fp = fopen(config_path, "r");
        if (config_path_fp == NULL)
        {
            continue;
        }
        while (fgets(config_line, sizeof(config_line), config_path_fp))
        {
            config_line[strcspn(config_line, "\n")] = 0;
            if (count == 0)
            {
                char * config_line_p = config_line;
                while (isspace(* config_line_p)) config_line_p++;
                if (* config_line_p != '@')
                {
                    break;
                }
                if (strcmp(package, config_line + 1) != 0)
                {
                    break;
                }
                end = 1;
                count++;
                continue;
            }
            
            pid_t newPid = fork();
            if (newPid == -1)
            {
                LOGPRINT(ANDROID_LOG_WARN, "AutoDisable", "Fork Error! Skip.\n");
                continue;
            }
            if (newPid == 0)
            {
                execlp("pm", "pm", arg, config_line, NULL);
                _exit(1);
            }
            else
            {
                int i = 0;
                if (waitpid(newPid, &i, 0) == -1)
                {
                    LOGPRINT(ANDROID_LOG_WARN, "AutoDisable", "Wait Error! Skip.\n");
                    continue;
                }
                if (WIFEXITED(i) && WEXITSTATUS(i) == 0)
                {
                    LOGPRINT(ANDROID_LOG_INFO, "AutoDisable", "%s: %s Success.\n", arg, config_line);
                }
                else
                {
                    LOGPRINT(ANDROID_LOG_WARN, "AutoDisable", "%s: %s Error! \n", arg, config_line);
                }
            }
        }
        fclose(config_path_fp);
    }
    closedir(config_dir_dp);
    
    return (end == 1) ? 0 : 1;
}
