Stegasis
========
Cambridge Part II Computer Science project <slw69@cam.ac.uk>

Useage
------
    # Prepare an existing video file
    $ stegasis format –-alg=lsb video.avi
     
    # Using stegasis mount we can directly mount the video file
    $ stegasis mount video.avi /mnt/volume
 
    # Create a file inside the file system
    $ echo "test" > /mnt/volume/test.txt
    # Unmount the file system
    $ stegasis umount /mnt/volume
