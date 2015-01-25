Stegasis
========
Cambridge Part II Computer Science project <slw69@cam.ac.uk>

Stegasis allows files to be steganographically hidden inside video files via a file system interface.

Stegasis provides a number of steganographic algorithms and encryption functionality. It currently natively supports uncompressed AVI video files, and via FFmpeg supports pretty much all other modern video formats (e.g. mp4, wmv, mkv etc.).

Example Usage
------
    # Prepare an existing video file
    $ stegasis format --alg=dctp --pass=password123 --cap=40 video.mp4
     
    # We can now mount the output video file
    $ stegasis mount --alg=dctp --pass=password123 video.mkv /mnt/volume
 
    # Create a file inside the file system
    $ echo "test" > /mnt/volume/test.txt
    # Copy a file into the file system
    $ cp ~/myfile.jpg /mnt/volume/
    # Unmount the file system
    $ stegasis umount /mnt/volume

Detailed Options
-----------------
    stegasis <command> [-p,-f] --alg=<alg> --pass=<pass> [--pass2=<pass2>] --cap=<capacity> <video_path> <mount_point>

Commands:
  * format  Formats a video for use with stegasis
  * mount  Mounts a formatted video to a given mount point

Flags:
  * --alg  Embedding algorithm to use, see below
  * --cap  Percentage of frame to embed within in percent
  * --pass  Passphrase used for encrypting and permuting data
  * --pass2 Pasphrase used for encrypting and permuting the hidden volume
  * -p  Do not flush writes to disk until unmount
  * -f  Force FFmpeg decoder to be used

Embedding Algorithms:
  * Uncompressed AVI only:
    * lsb: Least Significant Bit Sequential Embedding
    * lsbk: LSB Sequential Embedding XORd with a psudo random stream
    * lsbp: LSB Permuted Embedding using a seeded LCG
    * lsb2: Combination of lsbk and lsbp
  * Other video formats:
    * dctl: LSB Sequential Embedding within DCT coefficients
    * dctp: LSB Permuted Embedding within DCT coefficients
    * dct2: Combination of dctp and lsbk
    * dcta: LSB Permuted Embedding encrypted with AES
    * dct3: LSB Permuted Embedding encrypted with AES->Twofish->Serpent

Notes
------

To run Stegasis you will need the FUSE package installed <http://fuse.sourceforge.net>

sudo addgroup username fuse

Compile ffmpeg:
./configure --enable-static  --disable-shared --extra-libs=-static --extra-cflags=--static --enable-gpl --enable-version3 --enable-nonfree --enable-libopencore-amrnb --enable-libopencore-amrwb --enable-libfaac --enable-libgsm --enable-libmp3lame --enable-libtheora --enable-libvorbis --enable-libx264 --enable-libxvid

