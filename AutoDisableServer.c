// By FlyCome 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define WAITTIME 10

int main(int COMI, char * COM[])
{
    //检查当前用户权限
    uid_t nowuid = getuid();
    if (nowuid != 0 && nowuid != 2000)
    {
        printf(" » Please Use Root or Shell Run\n");
        return 0;
    }
    
    if (COMI < 2)
    {
        printf(" » 未传入配置路径！\n");
        return 1;
    }
    else
    {
        if (access(COM[1], F_OK) != 0)
        {
            printf(" » 配置路径不可访问或不存在！\n");
            return 1;
        }
    }
    
    char checkListFile[24] = "CheckList.conf";
    char disableListFile[24] = "DisableList.conf";
    
    char CHECKLIST_FILE[strlen(COM[1]) + 24];
    char DISABLELIST_FILE[strlen(COM[1]) + 24];
    
    snprintf(CHECKLIST_FILE, sizeof(CHECKLIST_FILE), "%s/%s", COM[1], checkListFile);
    snprintf(DISABLELIST_FILE, sizeof(DISABLELIST_FILE), "%s/%s", COM[1], disableListFile);
    
    if (access(CHECKLIST_FILE, F_OK) != 0)
    {
        printf(" » CheckList.conf 配置不存在/名称错误！\n");
        return 1;
    }
    else if (access(DISABLELIST_FILE, F_OK) != 0)
    {
        printf(" » DisableList.conf 配置不存在/名称错误！\n");
        return 1;
    }
    
    //定义循环所需变量
    int mode = 0;
    char oldTopApp[64] = "";
    
    //Start the cycle
    for ( ; ; )
    {
        //Get获取当前前台
        char TopApp[64] = "";
        FILE * TopApp_fp = popen("dumpsys window | grep mCurrentFocus | head -n 1 | cut -f 1 -d '/' | cut -f 5 -d ' ' | cut -f 1 -d ' '", "r");
        if (TopApp_fp == NULL)
        {
            printf(" » Get TopApp Err. Continue\n");
            continue;
        }
        fgets(TopApp, sizeof(TopApp), TopApp_fp);
        TopApp[strcspn(TopApp, "\n")] = 0;
        pclose(TopApp_fp);
        
        //检查屏幕状态
        if (strcmp(TopApp, "") == 0 ||
           strstr(TopApp, "NotificationShade") ||
           strstr(TopApp, "StatusBar") ||
           strstr(TopApp, "ActionsDialog"))
        {
            printf(" » Top is SystemUI. Wait\n");
            sleep(WAITTIME);
            continue;
        }
        
        //判断当前状态，1代表当前冻结列表App已冻结，待解冻
        if (mode == 1)
        {
            //这里判断当前前台是否仍然未变化保持监测状态
            if (strcmp(TopApp, oldTopApp) == 0)
            {
                printf(" » App Not Cycle. Wait\n");
                sleep(WAITTIME);
                continue;
            }
            else
            {
                //置2代表准备解冻App列表
                mode = 2;
            }
        }
        
        //读取检测列表并检测当前前台是否为被监测App
        char var[64] = "";
        FILE * checkList = fopen(CHECKLIST_FILE, "r");
        if (checkList)
        {
            while (fgets(var, sizeof(var), checkList))
            {
                var[strcspn(var, "\n")] = 0;
                                
                if (strcmp(var, TopApp) == 0)
                {
                    /*
                    如果当前前台是被监测 App 则置 mode=1 并保留包名以检测变化
                    这里不会影响前面置2的行为，因为如果前台仍然是被监测App就没必要解冻App列表
                    */
                    mode = 1;
                    snprintf(oldTopApp, sizeof(oldTopApp), "%s", var);
                    break;
                }
            }
            
            fclose(checkList);
        }
        else
        {
            printf(" » CheckList Read Err\n");
            return 1;
        }
        
        if (mode == 1) //为1则读取冻结App列表并逐一冻结
        {
            char disablePackage[64] = "";
            FILE * disableList = fopen(DISABLELIST_FILE, "r");
            if (disableList)
            {
                while (fgets(disablePackage, sizeof(disablePackage), disableList))
                {
                    disablePackage[strcspn(disablePackage, "\n")] = 0;
                    
                    char command[128] = "";
                    snprintf(command, sizeof(command), "pm disable-user %s >>/dev/null 2>&1", disablePackage);
                    
                    if (system(command) == 0)
                    {
                        printf(" » Disable: %s\n", disablePackage);
                    }
                    else
                    {
                        printf(" » Disable: %s Err\n", disablePackage);
                    }
                }
                
                fclose(disableList);
            }
            else
            {
                printf(" » DisableList Read Err\n");
                return 1;
            }
        }
        else if (mode == 2) //为2则读取冻结App列表并逐一解冻
        {
            char disablePackage[64] = "";
            FILE * disableList = fopen(DISABLELIST_FILE, "r");
            if (disableList)
            {
                while (fgets(disablePackage, sizeof(disablePackage), disableList))
                {
                    disablePackage[strcspn(disablePackage, "\n")] = 0;
                    
                    char command[128] = "";
                    snprintf(command, sizeof(command), "pm enable %s >>/dev/null 2>&1", disablePackage);
                    
                    if (system(command) == 0)
                    {
                        printf(" » Enable: %s\n", disablePackage);
                    }
                    else
                    {
                        printf(" » Enable: %s Err\n", disablePackage);
                    }
                }
                
                fclose(disableList);
            }
            else
            {
                printf(" » DisableList Read Err\n");
                return 1;
            }
            
            //解冻后重置此值
            mode = 0;
        }
        
        printf(" » Cycle Wait\n");
        sleep(WAITTIME);
        
    }
    
    return 0;
}
