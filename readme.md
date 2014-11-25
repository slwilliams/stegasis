Stegasis
========
Cambridge Part II Computer Science project <slw69@cam.ac.uk>

Stegasis allows files to be steganographically hidden inside video files via a file system interface.

Stegasis provides a number of steganographic algorithms and encryption functionality. It currently natively supports uncompressed AVI video files, and via FFmpeg supports pretty much all other modern video formats (e.g. mp4, wmv, mkv etc.).

Example Usage
------
    # Prepare an existing video file
    $ stegasis format –-alg=dctp --pass=password123 --cap=40 video.mp4
     
    # We can now mount the video file
    $ stegasis mount --alg=dctp --pass=password123 video.mp4 /mnt/volume
 
    # Create a file inside the file system
    $ echo "test" > /mnt/volume/test.txt
    # Copy a file into the file system
    $ cp ~/myfile.jpg /mnt/volume/
    # Unmount the file system
    $ stegasis umount /mnt/volume

Detailed Options
-----------------
Required Flags:
* --alg  Embedding algorithm to use, one of [lsb, lsbk, lsbp, lsb2, dctl, dctp]
* --cap  Percentage of frame to embed within in percent

Optional flags:
* --pass  Passphrase used for encrypting and permuting data, required for [lsbk, lsbp, lsb2, dctp]
* -p  Do not flush writes to disk until unmount
* -g  Use green channel for embedding

Notes
------

To run Stegasis you will need the FUSE package installed <http://fuse.sourceforge.net>

sudo addgroup <<username>> fuse

libc6-dev-i386

sudo apt-get install gcc-4.8-multilib g++-4.8-multilib
