# Project for Storage Technology

## setup

 - Ubuntu 16.04 
 - Download fuse-3.0.1 from: https://github.com/libfuse/libfuse/releases
 - install fuse 
     - Extract fuse-3.0.1.tar.gz
     - $ cd fuse-3.0.1
     - $ ./configure
     - $ make
     - $ sudo make install

## try to run tsinghua_learn_project

 - download src from: https://github.com/mxbhsa/learn_tsinghua_fuse
 - setup:
 	 - python 2.7
 	 - read the readme.pdf in tsinghua_learn_project to install python-c api and httplib2
 - modify src:
 	 - read the readme.pdf, modify 'learn.config'
 	 - learn.c:
 	 	 - modify `char bufPath[200]` and `char pythonPath[200]`
 	 	 - modify the function 'int getCourseList(struct User * user)', the path in 'PyRun_SimpleString("sys.path.append('/home/ywn/StoragePrj/learn/learn/')");' should be set correctly
 - $ mkdir fuse
 - $ mkdir fuse_temp
 - learn how to compile and run from readme.pdf

 ## learn how to use libfuse

  - 2 simple but useful demo: http://ouonline.net/building-your-own-fs-with-fuse-1
  - another demo as well as a tutorial: https://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/