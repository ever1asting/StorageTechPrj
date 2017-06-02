#define FUSE_USE_VERSION 26

#include "ftp/service.h"
#include "ftp/siftp.h"

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>  
#include <sys/stat.h>  
#include <unistd.h>  
#include <limits.h>
#include <sys/types.h> 

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


char bufBase[200] = "/home/ywn/StoragePrj/src/buffuse/";
// var used for socket
int m_socket;
char g_pwd[PATH_MAX+1];


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


// funcs for socket
Boolean service_create(int *ap_socket, const String a_serverName, const int a_serverPort)
{
  // variables
    struct sockaddr_in serverAddr;
    struct hostent *p_serverInfo;
    
  // create server address
    
    memset(&serverAddr, 0, sizeof(serverAddr));
    
    // determine dotted quad str from DNS query
      if((p_serverInfo = gethostbyname(a_serverName)) == NULL)
      {
        herror("service_create()");
        return false;
      }
      
      #ifndef NODEBUG
        printf("service_create(): serverName='%s', serverAddr='%s'\n", a_serverName, inet_ntoa(*((struct in_addr *)p_serverInfo->h_addr)));
      #endif
      
      serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)p_serverInfo->h_addr)));
      
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(a_serverPort);
    
  // create socket
    if((*ap_socket = socket(serverAddr.sin_family, SOCK_STREAM, 0)) < 0)
    {
      perror("service_create(): create socket");
      return false;
    }
  
  // connect on socket
    if((connect(*ap_socket, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr))) < 0)
    {
      perror("service_create(): connect socket");
      close(*ap_socket);
      return false;
    }
  
  return true;
}

Boolean session_create(const int a_socket)
{
  // variables
    Message msgOut, msgIn;
    
  // init vars
    Message_clear(&msgOut);
    Message_clear(&msgIn);
    
  // session challenge dialogue
  
    // cilent: greeting
    // server: identify
      Message_setType(&msgOut, SIFTP_VERBS_SESSION_BEGIN);
      
      if(!service_query(a_socket, &msgOut, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_IDENTIFY))
      {
        fprintf(stderr, "session_create(): connection request rejected.\n");
        return false;
      }
      
    // cilent: username
    // server: accept|deny
      Message_setType(&msgOut, SIFTP_VERBS_USERNAME);
      
      // get user input
        printf("\nusername: ");
        
        // XXX prohibited by this project
        //scanf("%s", msgOut.m_param);
      
      if(!service_query(a_socket, &msgOut, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_ACCEPTED))
      {
        fprintf(stderr, "session_create(): username rejected.\n");
        return false;
      }
    
    // cilent: password
    // server: accept|deny
    
      Message_setType(&msgOut, SIFTP_VERBS_PASSWORD);
      
      // get user input
        printf("\npassword: ");
        //scanf("%s", msgOut.m_param);
        strcpy(msgOut.m_param, "ce150");
       // = "ce150";

      if(!service_query(a_socket, &msgOut, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_ACCEPTED))
      {
        fprintf(stderr, "session_create(): password rejected.\n");
        return false;
      }
    
    // session now established
      #ifndef NODEBUG
        printf("session_create(): success\n");
      #endif
      
  return true;
}

int clientInit(int a_argc, char **ap_argv)
{
  // variables
    int socket;
    
  // init vars
    realpath(".", g_pwd);
    

    printf("break 1");
  // establish link
    if(!service_create(&socket, ap_argv[1], strtol(ap_argv[2], (char**)NULL, 10)))
    {
      fprintf(stderr, "%s: Connection to %s:%s failed.\n", ap_argv[0], ap_argv[1], ap_argv[2]);
      return -1;
    }
    

        printf("break 2");
  // establish session
    if(!session_create(socket))
    {
      close(socket);
      
      fprintf(stderr, "%s: Session failed.\n", ap_argv[0]);
      return -1;
    }
    
    printf("\nSession established successfully.");
  
    return socket;
  //  service_loop(socket);  
}

int split(char** dst, char* str, const char* spl)
{
    int n = 0;
    char *result = NULL;
    result = strtok(str, spl);
    while( result != NULL )
    {
        strcpy(dst[n++], result);
        result = strtok(NULL, spl);
    }
    return n;
}

int main(int argc, char *argv[])
{
    // init socket 
    int a_argc = 3;
    char *ap_argv[] = {"client", "127.0.0.1", "11800"};
    m_socket = clientInit(a_argc, ap_argv);
    if (m_socket == -1) {
      printf("create socket fail\n");
      exit(-1);
    }

    // init vars
    int i;
    char** filenameList = malloc(512 * sizeof(char*));
    for (i = 0; i < 512; ++i)
    filenameList[i] = malloc(512 * sizeof(char));                                   
    int* cnt = malloc(sizeof(int));
    *cnt = 0;

    // get list from server
    Message msgOut;
    String dataBuf;
    int dataBufLen;
    Boolean tempStatus = false;
    
    // init variables
    Message_clear(&msgOut);
    Message_setType(&msgOut, SIFTP_VERBS_COMMAND);
    Message_setValue(&msgOut, "ls");
      
    // perform command
    if(remote_exec(m_socket, &msgOut))
    {
      if((dataBuf = siftp_recvData(m_socket, &dataBufLen)) != NULL)
      {
        // printf("%s", dataBuf);
        *cnt = split(filenameList, dataBuf, "\n");
        free(dataBuf);
      }
      else {
        printf("error occurs when receive file list from server\n");
        exit(-1);
      }
    }

    printf("------------------------------\n");


    // listDir(filenameList, cnt, bufBase); 
    // printf("\n----------------\ncnt = %d\n-------------------\n", *cnt);

    struct file* f;
    for (i = 0; i < *cnt; ++i) {
        // get file size
        // char fullpath[1000];
        // strcpy(fullpath, bufBase);
        // strcat(fullpath, filenameList[i]);
        // FILE * fp = fopen(fullpath, "r");  
        // fseek(fp, 0L, SEEK_END);  
        // int size = ftell(fp);  
        // fclose(fp); 
        // init
        f = fileInit(filenameList[i], 0, 0);
        add2list(f);
    }
   // printf("break pt 1\n");

  	struct file* p = fileList;
  	for (p; p != NULL; p = p -> next) {
    		printf("file: %s, filepath = %s, bufpath = %s, size = %d, isDownload = %d\n",
    			p -> name, p -> filePath, p -> bufPath, p -> size, p -> isDownload);
  	}




    // destroy session
  //   if(session_destroy(socket))
  //     printf("Session terminated successfully.\n");
    
  // // destroy link
  //   close(socket);

   	return fuse_main(argc, argv, &fuse_example_operations, NULL);
}

