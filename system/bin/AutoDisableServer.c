// By FlyCome 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
    //Check ROOT 
    uid_t nowuid = getuid();
    if (nowuid != 0)
    {
        printf(" » Please use root privileges!\n");
        return 0;
    }
    
    //Get CheckAppPackage
    char CheckApp[128] = "";
    FILE * CheckApp_fp = popen("grep ^'检测=' /data/adb/modules/auto_disable/config.prop | cut -f2 -d '='", "r");
    fgets(CheckApp, sizeof(CheckApp), CheckApp_fp);
    CheckApp[strcspn(CheckApp, "\n")] = 0;
    pclose(CheckApp_fp);
    //Get DisableAppPackage
    char DisableApp[128] = "";
    FILE * DisableApp_fp = popen("grep ^'冻结=' /data/adb/modules/auto_disable/config.prop | cut -f2 -d '='", "r");
    fgets(DisableApp, sizeof(DisableApp), DisableApp_fp);
    DisableApp[strcspn(DisableApp, "\n")] = 0;
    pclose(DisableApp_fp);
    
    printf("CheckApp: %s\n", CheckApp);
    printf("DisableApp: %s\n", DisableApp);
    printf("\n");
    
    //Start the cycle
    for ( ; ; )
    {
        //Get Now App
        char NowPackageName[64] = "";
        FILE * NowPackageName_fp = popen("dumpsys window | grep mCurrentFocus | head -n 1 | cut -f 1 -d '/' | cut -f 5 -d ' ' | cut -f 1 -d ' '", "r");
        fgets(NowPackageName, sizeof(NowPackageName), NowPackageName_fp);
        NowPackageName[strcspn(NowPackageName, "\n")] = 0;
        pclose(NowPackageName_fp);
        
        //Check HW
        char checkhw[128] = "";
        snprintf(checkhw, sizeof(checkhw), "echo '%s' | grep StatusBar >>/dev/null 2>&1", NowPackageName);
        //Run
        int a = system(checkhw);
        if (a == 0)
        {
            printf("HW is Off\n");
            sleep(30);
            continue;
        }
        if (strcmp(NowPackageName, CheckApp) == 0)
        {
            printf("Disable App\n");
            //disable DisableApp
            char disable_command[128] = "";
            snprintf(disable_command, sizeof(disable_command), "pm disable %s >>/dev/null 2>&1", DisableApp);
            system(disable_command);
        }
        else
        {
            char Checkdisable_command[160] = "";
            snprintf(Checkdisable_command, sizeof(Checkdisable_command), "pm list packages -e | grep '%s' >>/dev/null 2>&1", DisableApp);
            
            int var = system(Checkdisable_command);
            if (var != 0)
            {
                printf("Enable App\n");
                char command[128] = "";
                snprintf(command, sizeof(command), "pm enable %s >>/dev/null 2>&1", DisableApp);
                system(command);
            }
            
            printf("Wait\n");
        }
        
        sleep(10);
        
    }
    
    return 0;
}



