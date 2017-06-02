#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>  
#include <sys/stat.h>  
#include <unistd.h>  
#include <sys/types.h> 

// static const char *filepath = "/file";
// static const char *filename = "file";
// static const char *filecontent = "I'm the content of the only file available there\n";
char bufBase[200] = "/home/ywn/StoragePrj/src/buffuse/";

struct file
{
	char name[256];
	char filePath[1000];
	char bufPath[1000]; 
	int size;
	int isDownload;
	struct file* next;
};

struct file* fileList = NULL;
void add2list(struct file* f)
{
	if (fileList == NULL) {
		fileList = f;
		return;
	}

	struct file* p = fileList;
	fileList = f;
	f -> next = p;
}

static int getattr_callback(const char *path, struct stat *stbuf) {
	memset(stbuf, 0, sizeof(struct stat));

  	if (strcmp(path, "/") == 0) {
    	stbuf->st_mode = S_IFDIR | 0755;
    	stbuf->st_nlink = 2;
    	return 0;
  	}

  	struct file* p = fileList;
  	while (p != NULL) {
  		if (strcmp(path, p -> filePath) == 0) {
  			stbuf -> st_mode = S_IFREG | 0777;
    		stbuf -> st_nlink = 1;
    		stbuf -> st_size = p -> size;
    		return 0;
  		}
  		p = p -> next;
  	}

 	return -ENOENT;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi) {
	(void) offset;
  	(void) fi;

  	if (strcmp(path, "/") == 0)	{
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);

		struct file* p = fileList;
  		while (p != NULL) {
  			filler(buf, p -> name, NULL, 0);
    		p = p -> next;
  		}
  	}

  	return 0;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
	printf("-----------------open-----------------\n");
	printf("path = %s\n", path);
	printf("------------------------------------\n");

	return 0;
}

void download(char* path)
{
	printf("download %s\n", path);
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {

	struct file* p = fileList;
	for (p; p != NULL; p = p -> next) {
		if (strcmp(path, p -> filePath) == 0) {
			break;
		}
	}

	// failure
	if (p == NULL) return 0;

	if (strlen(p -> bufPath) == 0) {
		// construct the buffer path and download
		char bufPath[1000];
		strcpy(bufPath, bufBase);
		strcat(bufPath, p -> name);
		strcpy(p -> bufPath, bufPath);

		download(bufPath);
		p -> isDownload = 1;

		// set file size
		FILE *fp = fopen(p -> bufPath, "r");
		if(!fp) {
			printf("failed to open the file from \"%s\"\n", p -> bufPath);
			return 0;
		}
		fseek(fp, 0L, SEEK_END);
		p -> size = ftell(fp);
		fclose(fp);
	}
	
	if (offset > p -> size)
		return 0;
	FILE *fp = fopen(p -> bufPath, "r");
	if(!fp) {
		printf("failed to open the file from \"%s\"\n", p -> bufPath);
		return 0;
	}
	fseek(fp, offset, SEEK_SET);
	if(offset + size > p -> size)
		size = p -> size - offset;
	size = fread(buf, sizeof(char), size, fp);
	fclose(fp);

	printf("*** has read %d offset:%d size:%d\n", (int)size, offset, p -> size);
	
	return size;

  // if (strcmp(path, filepath) == 0) {
  //   size_t len = strlen(filecontent);
  //   if (offset >= len) {
  //     return 0;
  //   }

  //   if (offset + size > len) {
  //     memcpy(buf, filecontent + offset, len - offset);
  //     return len - offset;
  //   }

  //   memcpy(buf, filecontent + offset, size);
  //   return size;
  // }

  // return -ENOENT;
}

int write_callback(const char *path, const char *buf, 
			size_t size, off_t offset, struct fuse_file_info *fi) {
    return 0;

    /*
    PSStatus *status = getPSStatus();

    for (int i=0; i<status->configsN; ++i)
        if (strcmp(path, status->configsPath[i]) == 0) {
            return status->configs[i]->write(buf, size, offset);
        }

    int ret = -ENOENT;
    for (int i=0; i<status->foldersN; ++i)
        if (strstr(path, status->foldersPath[i]) == path) {
            const char *fileName = path + strlen(status->foldersPath[i]) + 1;
            if ((strlen(fileName) > 0) && (status->fileMap[i].count(fileName))) {
                string newPath = status->bufferFolder;
                newPath += "/";
                newPath += status->fileMap[i][fileName];
                lseek(fi->fh, offset, SEEK_SET);
                ret = write(fi->fh, (void*)buf, size);
                return ret;
            }
        }
    return ret;
    */
}

int create_callback(const char *path , mode_t mode, struct fuse_file_info *fi) {
    return 0;
}

static struct fuse_operations fuse_example_operations = {
  .getattr = getattr_callback,
  .open = open_callback,
  .read = read_callback,
  .readdir = readdir_callback,
  .create = create_callback,
  .write = write_callback,
};

struct file* fileInit(const char* filename, int size, int isDownload)
{
	struct file* f = malloc (sizeof (struct file));

	strcpy(f -> name, filename);
	char filepath[1000] = "/";
	strcat(filepath, filename);
	strcpy(f -> filePath, filepath);

	if (isDownload) {
		char buf[1000];
		strcpy(buf, bufBase);
		strcat(buf, filename);
		strcpy(f -> bufPath, buf);
	}
	else 
		f -> bufPath[0] = '\0';

	f -> isDownload = isDownload;
	f -> size = size;

	return f;
}


void listDir(char** res, int* cnt, char *path)  
{  
    DIR              *pDir ;  
    struct dirent    *ent  ;  
    char              childpath[512];  

    pDir=opendir(path);  
    memset(childpath,0,sizeof(childpath));  

    while((ent=readdir(pDir))!=NULL)  
    {  

        if(ent->d_type & DT_DIR)  
        {  
			// do nothing with subfolder now

            if(strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0)  
                    continue;  

           // sprintf(childpath,"%s/%s",path,ent->d_name);  
           // printf("path:%s\n",childpath);   
        }  
        else
        {
            strcpy(res[*cnt], ent->d_name);
            res[*cnt][strlen(ent->d_name)] = '\0';
            *cnt += 1;
    	}
    }  
  
}  

int main(int argc, char *argv[])
{
    int i;
    char** filenameList = malloc(512 * sizeof(char*));
    for (i = 0; i < 512; ++i)
		filenameList[i] = malloc(512 * sizeof(char));                                   
    int* cnt = malloc(sizeof(int));
    *cnt = 0;
    listDir(filenameList, cnt, bufBase); 
    printf("\n----------------\ncnt = %d\n-------------------\n", *cnt);

    struct file* f;
    for (i = 0; i < *cnt; ++i) {
    	// get file size
    	char fullpath[1000];
    	strcpy(fullpath, bufBase);
    	strcat(fullpath, filenameList[i]);
	    FILE * fp = fopen(fullpath, "r");  
		fseek(fp, 0L, SEEK_END);  
		int size = ftell(fp);  
		fclose(fp); 
		// init
    	f = fileInit(filenameList[i], size, 1);
		add2list(f);
    }
   // printf("break pt 1\n");

	struct file* p = fileList;
	for (p; p != NULL; p = p -> next) {
		printf("file: %s, filepath = %s, bufpath = %s, size = %d, isDownload = %d\n",
			p -> name, p -> filePath, p -> bufPath, p -> size, p -> isDownload);
	}

 	return fuse_main(argc, argv, &fuse_example_operations, NULL);
}