Stegasis
========
Cambridge Part II Computer Science project <slw69@cam.ac.uk>

Stegasis allows files to be steganographically hidden inside video files via a file system interface.

Stegasis provides a number of steganographic algorithms and encryption functionality. It currently natively supports uncompressed AVI video files, and via FFmpeg supports pretty much all other modern video formats (e.g. mp4, wmv, mkv etc.).

Example Usage
------
    # Prepare an existing video file
    $ stegasis format --alg=dctp --pass=password123 --cap=40 video.mp4
     
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
    stegasis <command> [-p,-g] --alg=<alg> --pass=<pass> --cap=<capacity> <video_path> <mount_point>
Commands:
* format  Formats a video for use with stegasis
* mount  Mounts a formatted video to a given mount point
Flags:
* --alg  Embedding algorithm to use, see below
* --cap  Percentage of frame to embed within in percent
* --pass  Passphrase used for encrypting and permuting data
* -p  Do not flush writes to disk until unmount
* -f  Force FFmpeg decoder to be used
Embedding Algorithms:
* Uncompressed AVI only:
..* lsb: Least Significant Bit Sequential Embedding
..* lsbk: LSB Sequential Embedding XOR'd with a psudo random stream
..* lsbp: LSB Permuted Embedding using a seeded LCG
..* lsb2: Combination of lsbk and lsbp
* Other video formats:
..* dctl: LSB Sequential Embedding within DCT coefficients
..* dctp: LSB Permuted Embedding within DCT coefficients
..* dct2: Combination of dctp and lsbk
..* dcta: LSB Permuted Embedding encrypted with AES
..* dct3: LSB Permuted Embedding encrypted with AES->Twofish->Serpent

Notes
------

To run Stegasis you will need the FUSE package installed <http://fuse.sourceforge.net>

sudo addgroup username fuse

If you experience audio sync issues run the following command to convert your video to 25 FPS:

ffmpeg -i input.mp4 -qscale 0 -r 25 -y output.mp4

libc6-dev-i386

sudo apt-get install gcc-4.8-multilib g++-4.8-multilib
