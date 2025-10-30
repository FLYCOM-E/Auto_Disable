// By FlyCome 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
    //检查当前用户权限
    uid_t nowuid = getuid();
    if (nowuid != 0 && nowuid != 2000)
    {
        printf(" » Please Use Root or Shell Run\n");
        return 0;
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
            printf(" » Get TopApp Err\n");
            sleep(10);
            continue;
        }
        fgets(TopApp, sizeof(TopApp), TopApp_fp);
        TopApp[strcspn(TopApp, "\n")] = 0;
        pclose(TopApp_fp);
        
        //这里检查屏幕状态
        if (strstr(TopApp, "NotificationShade") ||
           strstr(TopApp, "StatusBar") ||
           strstr(TopApp, "ActionsDialog"))
        {
            printf(" » Top is SystemUI. Wait\n");
            sleep(10);
            continue;
        }
        
        //判断当前状态，1代表当前冻结列表App已冻结，待解冻
        if (mode == 1)
        {
            //这里判断当前前台是否仍然未变化保持监测状态
            if (strcmp(TopApp, oldTopApp) == 0)
            {
                printf(" » App Not Cycle. Wait\n");
                sleep(10);
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
        FILE * checkList = fopen("CheckList.conf", "r");
        if (checkList)
        {
            while (fgets(var, sizeof(var), checkList))
            {
                var[strcspn(var, "\n")] = 0;
                                
                if (strcmp(var, TopApp) == 0)
                {
                    /*
                    如果当前前台是被监测App则置1并保留此包名以检测变化
                    这里不会影响前面置2的行为，因为如果前台仍然是被监测App
                    就没必要解冻App列表
                    */
                    mode = 1;
                    strcpy(oldTopApp, var);
                    break;
                }
            }
            
            fclose(checkList);
        }
        else
        {
            printf(" » CheckList.conf Read Err\n");
            return 1;
        }
        
        if (mode == 1) //为1则读取冻结App列表并逐一冻结
        {
            char disablePackage[64] = "";
            FILE * disableList = fopen("DisableList.conf", "r");
            if (disableList)
            {
                while (fgets(disablePackage, sizeof(disablePackage), disableList))
                {
                    disablePackage[strcspn(disablePackage, "\n")] = 0;
                    
                    char command[128] = "";
                    snprintf(command, sizeof(command), "pm disable %s >>/dev/null 2>&1", disablePackage);
                    
                    int i = system(command);
                    if (i == 0)
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
                printf(" » DisableList.conf Read Err\n");
                return 1;
            }
        }
        else if (mode == 2) //为2则读取冻结App列表并逐一解冻
        {
            char disablePackage[64] = "";
            FILE * disableList = fopen("DisableList.conf", "r");
            if (disableList)
            {
                while (fgets(disablePackage, sizeof(disablePackage), disableList))
                {
                    disablePackage[strcspn(disablePackage, "\n")] = 0;
                    
                    char command[128] = "";
                    snprintf(command, sizeof(command), "pm enable %s >>/dev/null 2>&1", disablePackage);
                    
                    int i = system(command);
                    if (i == 0)
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
                printf(" » DisableList.conf Read Err\n");
                return 1;
            }
            
            //解冻后重置此值
            mode = 0;
        }
        
        printf(" » Cycle Wait\n");
        sleep(10);
        
    }
    
    return 0;
}
