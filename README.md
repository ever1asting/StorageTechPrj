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

 ## learn how to use libfuse

  - 2 simple but useful demo: http://ouonline.net/building-your-own-fs-with-fuse-1
  - another demo as well as a tutorial: https://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/

## Usage for OnlineDisk

 - cd src/ftpLib, make -> Server
 - cd src, make -> client
 - to run the server, in ftpLib folder, `./siftpd <Server Folder> <Port Number>`
 - to run the client, in src folder, `./main <Mount Folder>`, and add parameter [-d] for debug
 - make sure <Server Folder> and <Mount Folder> exit before programs run
 - DO NOT OPERATE TOO FAST