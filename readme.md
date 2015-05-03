Stegasis
========
Cambridge Part II Computer Science project <slw69@cam.ac.uk>

Stegasis allows files to be steganographically hidden inside video files via a file system interface.

Stegasis provides a number of steganographic algorithms and encryption functionality. It currently natively supports uncompressed AVI video files, and via FFmpeg supports pretty much all other modern video formats (e.g. mp4, wmv, mkv etc.).

Example Usage
------
    # Prepare an existing video file
    $ stegasis format --alg=dctp --crypt=aes --pass=password123 --cap=40 video.mp4
     
    # We can now mount the output video file
    $ stegasis mount --alg=dctp --crypt=aes --pass=password123 video.mkv /mnt/volume
 
    # Create a file inside the file system
    $ echo "test" > /mnt/volume/test.txt
    # Copy a file into the file system
    $ cp ~/myfile.jpg /mnt/volume/
	
    # Unmount the file system
    <Ctrl-C>

Detailed Options
-----------------
    stegasis <command> [-p,-f] --alg=<alg> [--crypt=<alg>] --pass=<pass> [--pass2=<pass2>] --cap=<capacity> <video_path> <mount_point>

Commands:
  * format  Formats a video for use with Stegasis
  * mount  Mounts a formatted video to a given mount point

Required Flags:
  * --alg  Embedding algorithm to use, see below
  * --pass  Passphrase used for encrypting and permuting data

Optional Flags:
  * --cap  Percentage of frame to embed within in percent
  * --crypt  Cryptographic algorithm used to encrypt embedded data
  * --pass2 Pasphrase used for encrypting and permuting the hidden volume
  * -p  Do not flush writes to disk until unmount
  * -f  Force FFmpeg decoder to be used

Embedding Algorithms:
  * Uncompressed AVI only:
    * lsb: Least Significant Bit Sequential Embedding
    * lsbp: LSB Permuted Embedding using a seeded LCG
  * Other video formats:
    * dctl: LSB Sequential Embedding within DCT coefficients
    * dctp: LSB Permuted Embedding within DCT coefficients
    * f4:  Implementation of the F4 embedding algorithm
	* f5:  Implementation of the F5 algorithm

Cryptographic Algorithms:
  * aes:  256 bit AES (Rijndael)
  * twofish:  256 bit TwoFish
  * serpent:  256 bit Serpent
  * aes_serpent_twofish:  Chained combination of the above 3

Notes
------

To run Stegasis you will need the FUSE package installed <http://fuse.sourceforge.net>

sudo addgroup username fuse

Compile ffmpeg:
./configure --enable-static  --disable-shared --extra-libs=-static --extra-cflags=--static --enable-gpl --enable-version3 --enable-nonfree --enable-libopencore-amrnb --enable-libopencore-amrwb --enable-libfaac --enable-libgsm --enable-libmp3lame --enable-libtheora --enable-libvorbis --enable-libx264 --enable-libxvid

License
---------

Stegasis is licensed under the standard MIT License.

Copyright (C) 2015 by Scott Williams

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.