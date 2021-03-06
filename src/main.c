#define FUSE_USE_VERSION 26

#include "ftpLib/service.h"
#include "ftpLib/siftp.h"

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
char bufBase[200] = "/home/ywn/StoragePrj/src/buffuse/";
// var used for socket
int m_socket;
char g_pwd[PATH_MAX+1];


// functions declaration
struct file* fileInit(const char* filename, int size, int isDownload);
Boolean download(char* name, char* path);
void updateFileList();


// **********************************************************
// here are the functions help to achieve <syscall_callback>
// **********************************************************
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

void updateFileList()
{
	// clean prev filelist
    fileList = NULL;

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
    
    Message_clear(&msgOut);
    Message_setType(&msgOut, SIFTP_VERBS_COMMAND);
    Message_setValue(&msgOut, "sizels");
      
    if(remote_exec(m_socket, &msgOut))
    {
		if((dataBuf = siftp_recvData(m_socket, &dataBufLen)) != NULL)
		{
			printf("dataBuf, %s", dataBuf);
			*cnt = split(filenameList, dataBuf, "\n");
			free(dataBuf);
		}
		else {
			printf("error occurs when receive file list from server\n");
			exit(-1);
		}
    }

    printf("------------------------------\n");
    printf("cnt = %d\n", *cnt);

    // construct filelist
    struct file* f;
    for (i = 0; i < *cnt; ++i) {
        int j;
        char** tempName = malloc(4 * sizeof(char*));
        for (j = 0; j < 4; ++j)
			tempName[j] = malloc(512 * sizeof(char)); 
        split(tempName, filenameList[i], " ");
        strcpy(filenameList[i], tempName[1]);

        f = fileInit(filenameList[i], atoi(tempName[0]), 0);
        add2list(f);

        for (j = 0; j < 4; ++j)
			free(tempName[j]);
        free(tempName); 
    }

    // show new filelist
    struct file* p = fileList;
    for (p; p != NULL; p = p -> next) {
        printf("file: %s, filepath = %s, bufpath = %s, size = %d, isDownload = %d\n",
          p -> name, p -> filePath, p -> bufPath, p -> size, p -> isDownload);
    }
}

Boolean download(char* name, char* path)
{
    printf("download %s\n", path);

    char dstPath[PATH_MAX+1];
    String src, dst;
    Message msgOut;
    String dataBuf;
    int dataBufLen;
    Boolean tempStatus = false;
    
    Message_clear(&msgOut);
    src = name;
    dst = path;
    
    // build command with param='get remote-path'
    Message_setType(&msgOut, SIFTP_VERBS_COMMAND);
    Message_setValue(&msgOut, "get");
    strcat(Message_getValue(&msgOut), " ");
    strcat(Message_getValue(&msgOut), src);
      
    // determine destination file path
	if(service_getAbsolutePath(g_pwd, dst, dstPath)) {
        // check write perms & file type
        if(service_permTest(dstPath, SERVICE_PERMS_WRITE_TEST) 
        	&& service_statTest(dstPath, S_IFMT, S_IFREG)) {
          	// perform command
          	if(remote_exec(m_socket, &msgOut)) {
            	// receive destination file
            	if((dataBuf = siftp_recvData(m_socket, &dataBufLen)) != NULL) {
              		// write file
              		if((tempStatus = service_writeFile(dstPath, dataBuf, dataBufLen)))
                		printf("%d bytes transferred.", dataBufLen);
          
					free(dataBuf);
              
					#ifndef NODEBUG
						printf("get(): file writing %s.\n", tempStatus ? "OK" : "FAILED");
					#endif
            	}
	            #ifndef NODEBUG
	            else
					printf("get(): getting of remote file failed.\n");
	            #endif
          	}
			#ifndef NODEBUG
			else
				printf("get(): server gave negative ACK.\n");
			#endif
        }
        #ifndef NODEBUG
        else
			printf("get(): don't have write permissions.\n");
        #endif
	}
	#ifndef NODEBUG
	else
		printf("get(): absolute path determining failed.\n");
	#endif

    return tempStatus;
}

Boolean upload(char* name, char* path)
{
    printf("upload %s\n", path);
    char srcPath[PATH_MAX + 1];
    String src, dst;

    // construct msg
    Message msgOut;
    String dataBuf;
    int dataBufLen;
    Boolean tempStatus = false;    
    Message_clear(&msgOut);
    src = path;
    dst = name;
      
	// build command with param='put remote-path'
	Message_setType(&msgOut, SIFTP_VERBS_COMMAND);
	Message_setValue(&msgOut, "put");
	strcat(Message_getValue(&msgOut), " ");
	strcat(Message_getValue(&msgOut), dst);
  
    // determine source path
	if(service_getAbsolutePath(g_pwd, src, srcPath)) {
		// check read perms & file type
		if(service_permTest(srcPath, SERVICE_PERMS_READ_TEST) && 
				service_statTest(srcPath, S_IFMT, S_IFREG)) {
			// try to read source file
			if((dataBuf = service_readFile(srcPath, &dataBufLen)) != NULL) {
				// client: i'm sending a file
				if(remote_exec(m_socket, &msgOut)) {
					if((tempStatus = (siftp_sendData(m_socket, dataBuf, dataBufLen) 
						&& service_recvStatus(m_socket)))) {
						// server: success
						printf("%d bytes transferred.", dataBufLen);
					}

					#ifndef NODEBUG
						printf("put(): file sent %s.\n", tempStatus ? "OK" : "FAILED");
					#endif
				}
				#ifndef NODEBUG
				else
					printf("put(): server gave negative ACK.\n");
				#endif

					free(dataBuf);
				}
				#ifndef NODEBUG
				else
					printf("put(): file reading failed.\n");
				#endif
			}
			#ifndef NODEBUG
			else
				printf("put(): don't have read permissions.\n");
			#endif
	}
	#ifndef NODEBUG
	else
		printf("put(): absolute path determining failed.\n");
	#endif
  
    return tempStatus;
}

struct file* fileInit(const char* filename, int size, int isDownload)
{
  	struct file* f = malloc (sizeof (struct file));

  	strcpy(f -> name, filename);
  	char filepath[1000] = "/";
  	strcat(filepath, filename);
  	strcpy(f -> filePath, filepath);

    // construct the buf path
	char buf[1000];
	strcpy(buf, bufBase);
	strcat(buf, filename);
	strcpy(f -> bufPath, buf);

  	f -> isDownload = isDownload;
  	f -> size = size;

  	return f;
}


void listDir(char** res, int* cnt, char *path)  
{  
    DIR* pDir;  
    struct dirent* ent;  
    char childpath[512];  

    pDir=opendir(path);  
    memset(childpath,0,sizeof(childpath));  

    while((ent = readdir(pDir)) != NULL)  
    {  

        if(ent->d_type & DT_DIR)  
        {  
			// do nothing with subfolder now
            if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)  
                    continue;    
        }  
        else
        {
            strcpy(res[*cnt], ent->d_name);
            res[*cnt][strlen(ent->d_name)] = '\0';
            *cnt += 1;
        }
    }  
}  

// **********************************************************
// here are functions used by socket
// **********************************************************
Boolean service_create(int *ap_socket, const String a_serverName, const int a_serverPort)
{
	struct sockaddr_in serverAddr;
    struct hostent *p_serverInfo; 
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
  	
  	// parameters for socket
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
	Message msgOut, msgIn;
	Message_clear(&msgOut);
	Message_clear(&msgIn);
	Message_setType(&msgOut, SIFTP_VERBS_SESSION_BEGIN);

	// identify
	if(!service_query(a_socket, &msgOut, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_IDENTIFY))
	{
		fprintf(stderr, "session_create(): connection request rejected.\n");
		return false;
	}

	//check the username and password
	Message_setType(&msgOut, SIFTP_VERBS_USERNAME);
	printf("\nusername: ");
	if(!service_query(a_socket, &msgOut, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_ACCEPTED))
	{
		fprintf(stderr, "session_create(): username rejected.\n");
		return false;
	}

	Message_setType(&msgOut, SIFTP_VERBS_PASSWORD);
	printf("\npassword: ");
	strcpy(msgOut.m_param, "ce150");
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
    int socket;
    realpath(".", g_pwd);
   	
   	// establish link
    if(!service_create(&socket, ap_argv[1], strtol(ap_argv[2], (char**)NULL, 10)))
    {
		fprintf(stderr, "%s: Connection to %s:%s failed.\n", ap_argv[0], ap_argv[1], ap_argv[2]);
		return -1;
	}
  	// establish session
    if(!session_create(socket))
    {
		close(socket);
		fprintf(stderr, "%s: Session failed.\n", ap_argv[0]);
		return -1;
    }
    
    printf("\nSession established successfully.");
  
    return socket;
}



// **********************************************************
// here are <syscall_callback> functions
// **********************************************************
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

	struct file* p = fileList;
	for (p; p != NULL; p = p -> next) {
		if (strcmp(p -> name, path + 1) == 0) {
			if (p -> isDownload == 0) {
				// need to download file from server
				if (download(p -> name, p -> bufPath) == false) {
					printf("failed to download %s from server.\n");
					return 0;
				}
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

			int ret = open(p -> bufPath, O_RDWR);
			if (ret < 0)
				return -1;
			close(ret);
			return 0;
		}
	}

	return 0;
}



static int read_callback(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {

	printf("----\nread\n%s, size = %d, offset= %d\n-------\n", 
            	path, size, offset);

	// search in filelist
	struct file* p = fileList;
	for (p; p != NULL; p = p -> next) {
		if (strcmp(path, p -> filePath) == 0) {
			break;
		}
	}

	// failure
	if (p == NULL) return 0;

	if (p -> isDownload == 0) {
		// construct the buffer path and download
		char bufPath[1000];
		strcpy(bufPath, bufBase);
		strcat(bufPath, p -> name);
		strcpy(p -> bufPath, bufPath);

		if (download(p -> name, bufPath) == false) {
        	printf("failed to download %s from server.\n");
        	return 0;
    	}
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

	// read
	int fd = open(p -> bufPath, O_RDONLY);
	lseek(fd, offset, SEEK_SET);
	int ret = read(fd, (void*)buf, size);
	close(fd);

	printf("function read, size =  %d offset:%d p -> size:%d\n",
			 (int)size, offset, p -> size);
	
	return size;
}

int write_callback(const char *path, const char *buf, 
			size_t size, off_t offset, struct fuse_file_info *fi) {

    printf("-----------\nwrite\n--------------------\n");
    printf("path = %s\n", path);
    printf("buf = %s\n");

    // write to bufPath
    char tempBufPath[1000];
    strcpy(tempBufPath, bufBase);
    strcat(tempBufPath, path + 1);
    int fd = open(tempBufPath, O_WRONLY);
    lseek(fd, offset, SEEK_SET);
    int ret = write(fd, (void*)buf, size);
    close(fd);

    // upload to server
    if (upload(path + 1, tempBufPath) == false) {
        printf("failed to upload\n");
        return 0;   
    }
    
    // modify fileList
    struct file* p = fileList;
    for (p; p != NULL; p = p -> next) {
      if (strcmp(p -> name, path + 1) == 0) {
        p -> isDownload = 1;
        p -> size = size;
        break;
      }
    }
    if (p == NULL)
      add2list(fileInit(path + 1, size, 1));

    printf("function write, size:%d offset:%d\n",(int)size, offset);
    
    return size;
}

int create_callback(const char *path , mode_t mode, struct fuse_file_info *fi) {
    printf("----\ncreate\n---------------\n");
    printf("%s\n", path);

    struct file* p = fileList;
    for (p; p != NULL; p = p -> next) {
        if (strcmp(path + 1, p -> name) == 0)
            return -EEXIST;        
    }

    struct file* f = fileInit(path + 1, 0, 0);
    add2list(f);
    int ret = open(f -> bufPath, (int)mode | O_CREAT);
    if (ret >= 0) 
        close(ret);

    return 0;
}

int opendir_callback(const char *path, struct fuse_file_info *fi) {
    printf("-------\nopendir, %s\n---------\n");

    if (strcmp(path, "/") == 0) {
      updateFileList();
    }

    return 0;
}

static struct fuse_operations fuse_example_operations = {
	.getattr = getattr_callback,
	.open = open_callback,
	.read = read_callback,
	.readdir = readdir_callback,
	.create = create_callback,
	.write = write_callback,
	.opendir = opendir_callback,
};



int main(int argc, char *argv[])
{
    // init socket 
    int a_argc = 3;
    // ip and port is fixed, for convenience of test
    char *ap_argv[] = {"client", "127.0.0.1", "11800"};
    m_socket = clientInit(a_argc, ap_argv);
    if (m_socket == -1) {
      printf("create socket fail\n");
      exit(-1);
    }

    updateFileList();

   	return fuse_main(argc, argv, &fuse_example_operations, NULL);
}

