#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

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
	return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {

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

  return -ENOENT;
}

static struct fuse_operations fuse_example_operations = {
  .getattr = getattr_callback,
  .open = open_callback,
  .read = read_callback,
  .readdir = readdir_callback,
};

struct file* fileInit(const char* filename, int size, int isDownload)
{
	 struct file* f = malloc (sizeof (struct file));
	 
	 strcpy(f -> name, filename);
	 
	 char filepath[1000] = "/";
	 strcat(filepath, filename);
	 strcpy(f -> filePath, filepath);

	 char bufPath[1000];
	 strcpy(bufPath, bufBase);
	 strcat(bufPath, filename);
	 strcpy(f -> bufPath, bufPath);

	 f -> isDownload = isDownload;
	 f -> size = size;

	 return f;
}


int main(int argc, char *argv[])
{
	struct file* f = fileInit("fileA", 12345, 0);
	add2list(f);
	f = fileInit("fileB", 12222, 0);
	add2list(f);
	f = fileInit("fileC", 12333, 1);
	add2list(f);

	struct file* p = fileList;
	for (p; p != NULL; p = p -> next) {
		printf("file: %s, filepath = %s, bufpath = %s, size = %d, isDownload = %d\n",
			p -> name, p -> filePath, p -> bufPath, p -> size, p -> isDownload);
	}

 	return fuse_main(argc, argv, &fuse_example_operations, NULL);
}