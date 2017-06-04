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

## About the lib for FTP
 - the source codes under `src/ftpLib` folder are based on `Suraj Kurapati <skurapat@ucsc.edu>, CMPE-150, Spring 2004, Network Programming Project`, you can see the original codes at `https://github.com/sunaku/simple-ftp`
 - The original project has a fantastic architecture. However, it can only transmit text file. That's to say, when it comes to binary file, it fails.
 - Thus I modify some codes in order to transmit both text files and binary files.
 - And that's what you see in `src/ftpLib` folder actually.