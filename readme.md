Stegasis
========
Cambridge Part II Computer Science project <slw69@cam.ac.uk>
Stegasis allows files to be steganographically hidden inside video files via a file system interface.
Stegasis provides a number of steganographic algorithms, encryption functionality and (currently) supports uncompressed AVI video files.

Useage
------
    # Prepare an existing video file
    $ stegasis format –-alg=lsbk --pass=password123 video.avi
     
    # Using stegasis mount we can directly mount the video file
    $ stegasis mount --alg=lsbk --pass=password123 video.avi /mnt/volume
 
    # Create a file inside the file system
    $ echo "test" > /mnt/volume/test.txt
    # Copy a file into the file system
    $ cp ~/myfile.jpg /mnt/volume/
    # Unmount the file system
    $ stegasis umount /mnt/volume

Notes
------

To build Stegasis from source you will need the FUSE package installed <http://fuse.sourceforge.net>

libc6-dev-i386

sudo apt-get install gcc-4.8-multilib g++-4.8-multilib
