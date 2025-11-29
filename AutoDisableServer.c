// By FlyCome 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define DEBUG 0
#define WAITTIME 10
#define MAX_PACKAGE 128
#define CHECKLIST "CheckList.conf"
#define DISABLELIST "DisableList.conf"
#define GET_TOPAPP "dumpsys window | grep mCurrentFocus | head -n 1 | cut -f 1 -d '/' | cut -f 5 -d ' ' | cut -f 1 -d ' '"

static int service(int mode, char * disablelist_file);

int main(int COMI, char * COM[])
{
    if (getuid() != 0 && getuid() != 2000) //检查当前用户权限
    {
        printf(" » Please Use Root or Shell Run\n");
        return 0;
    }
    if (COMI < 2) //检查参数数量
    {
        printf(" » 未传入配置路径！\n");
        return 1;
    }
    else if (access(COM[1], F_OK) != 0) //检查路径能否访问
    {
        printf(" » 配置路径不可访问或不存在！\n");
        return 1;
    }
    
    // 拼接完整配置文件路径并检查
    char checklist_file[strlen(COM[1]) + 24];
    char disablelist_file[strlen(COM[1]) + 24];
    snprintf(checklist_file, sizeof(checklist_file), "%s/%s", COM[1], CHECKLIST);
    snprintf(disablelist_file, sizeof(disablelist_file), "%s/%s", COM[1], DISABLELIST);
    
    // 检查 CheckList、DisableList存在性
    if (access(checklist_file, F_OK) != 0)
    {
        printf(" » %s 配置不存在/名称错误！\n", CHECKLIST);
        return 1;
    }
    else if (access(disablelist_file, F_OK) != 0)
    {
        printf(" » %s 配置不存在/名称错误！\n", DISABLELIST);
        return 1;
    }
    
    if (DEBUG == 0) // 非Debug模式则脱离控制端启动并忽略输出
    {
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
    }
    
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
                printf(" » Get TopApp Err. Timeout...\n");
                break;
            }
            printf(" » Get TopApp Err. Continue\n");
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
            printf(" » Top is SystemUI. Wait\n");
            sleep(WAITTIME);
            continue;
        }
        
        //判断当前状态，置 1 ：当前冻结列表App已冻结，待解冻
        if (mode == 1)
        {
            // 这里检查当前前台是否未变化
            if (strcmp(top_app, old_top_app) == 0)
            {
                printf(" » App Not Cycle. Wait\n");
                sleep(WAITTIME);
                continue;
            }
            else
            {
                mode = 2; // 置 2 准备解冻 App 列表
            }
        }
        
        // 读取检测列表并检测当前前台 App 是否在该列表
        char checklist_line[MAX_PACKAGE] = "";
        FILE * checklist_file_fp = fopen(checklist_file, "r");
        if (checklist_file_fp)
        {
            while (fgets(checklist_line, sizeof(checklist_line), checklist_file_fp))
            {
                checklist_line[strcspn(checklist_line, "\n")] = 0;
                if (strcmp(checklist_line, top_app) == 0)
                {
                    /* 如果当前前台是被监测 App 则置 mode=1 并保留包名以检测变化
                    这里不会影响前面置2的行为，因为如果前台仍然是被监测App就没必要解冻App列表 */
                    mode = 1;
                    snprintf(old_top_app, sizeof(old_top_app), "%s", checklist_line);
                    break;
                }
            }
            fclose(checklist_file_fp);
        }
        else
        {
            printf(" » %s Read Err\n", CHECKLIST);
            break;
        }
        
        // Service
        if (service(mode, disablelist_file) == 0)
        {
            if (mode == 2) mode = 0; //解冻后重置此值
        }
        else
        {
            // 失败检查 CheckList、DisableList
            if (access(checklist_file, F_OK) != 0)
            {
                printf(" » %s 配置已不存在！\n", CHECKLIST);
                return 1;
            }
            else if (access(disablelist_file, F_OK) != 0)
            {
                printf(" » %s 配置已不存在！\n", DISABLELIST);
                return 1;
            }
        }
        
        printf(" » Cycle Wait\n");
        sleep(WAITTIME);
    }
    return 0;
}

/*
mode 为 1 则读取冻结 App 列表并逐一冻结
mode 为 2 则读取冻结 App 列表并逐一解冻
返回 0 代表成功，127 代表列表打开失败
*/
static int service(int mode, char * disablelist_file)
{
    int end = 0;
    char disablelist_line[MAX_PACKAGE] = "";
    FILE * disablelist_file_fp = fopen(disablelist_file, "r");
    if (disablelist_file_fp)
    {
        if (mode == 1)
        {
            while (fgets(disablelist_line, sizeof(disablelist_line), disablelist_file_fp))
            {
                disablelist_line[strcspn(disablelist_line, "\n")] = 0;
                pid_t newPid = fork();
                if (newPid == -1)
                {
                    printf(" » DisableService Fork Error!\n");
                    continue;
                }
                if (newPid == 0)
                {
                    execlp("pm", "pm", "disable-user", disablelist_line, NULL);
                    _exit(1);
                }
                else
                {
                    int i = 0;
                    if (waitpid(newPid, &i, 0) == -1)
                    {
                        printf(" » Wait DisableService Error!\n");
                        continue;
                    }
                    if (WIFEXITED(i) && WEXITSTATUS(i) == 0)
                    {
                        printf(" » Disable: %s\n", disablelist_line);
                    }
                    else
                    {
                        printf(" » Disable: %s Err\n", disablelist_line);
                    }
                }
            }
        }
        else if (mode == 2)
        {
            while (fgets(disablelist_line, sizeof(disablelist_line), disablelist_file_fp))
            {
                disablelist_line[strcspn(disablelist_line, "\n")] = 0;
                pid_t newPid = fork();
                if (newPid == -1)
                {
                    printf(" » EnableService Fork Error!\n");
                    continue;
                }
                if (newPid == 0)
                {
                    execlp("pm", "pm", "enable", disablelist_line, NULL);
                    _exit(1);
                }
                else
                {
                    int i = 0;
                    if (waitpid(newPid, &i, 0) == -1)
                    {
                        printf(" » Wait EnableService Error!\n");
                        continue;
                    }
                    if (WIFEXITED(i) && WEXITSTATUS(i) == 0)
                    {
                        printf(" » Enable: %s\n", disablelist_line);
                    }
                    else
                    {
                        printf(" » Enable: %s Err\n", disablelist_line);
                    }
                }
            }
        }
    }
    else
    {
        end = 127;
    }
    
    fclose(disablelist_file_fp);
    return end;
}
