#include <stdio.h>  
#include <string.h> 
#include <stdlib.h>  
#include <dirent.h>  
#include <sys/stat.h>  
#include <unistd.h>  
#include <sys/types.h> 


void listDir(char** res, int* cnt, char *path)  
{  
        DIR              *pDir ;  
        struct dirent    *ent  ;  
        int               i=0  ;  
        char              childpath[512];  
  
        pDir=opendir(path);  
        memset(childpath,0,sizeof(childpath));  
  
//        int cnt = 0;
        while((ent=readdir(pDir))!=NULL)  
        {  
  
                if(ent->d_type & DT_DIR)  
                {  
  
                        if(strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0)  
                                continue;  
  
                        sprintf(childpath,"%s/%s",path,ent->d_name);  
                        printf("path:%s\n",childpath);  
  
                      //  listDir(childpath);  
  
                }  
                else
                {
                        strcpy(res[*cnt], ent->d_name);
                        res[*cnt][strlen(ent->d_name)] = '\0';
                        *cnt += 1;
                        //printf("%s\n", ent->d_name);
                //cout<<<<endl;
                }
        }  
  
}  
  
int main(int argc,char *argv[])  
{  
        int i;
        char** flist = malloc(512 * sizeof(char*));
        for (i = 0; i < 512; ++i)
                flist[i] = malloc(512 * sizeof(char));                                   

        int* cnt = malloc(sizeof(int));
        *cnt = 0;
        listDir(flist, cnt, argv[1]); 
        printf("cnt = %d\n", *cnt);
        for (i = 0; i < *cnt; ++i)
                printf("%s\n", flist[i]);

        return 0;  
}