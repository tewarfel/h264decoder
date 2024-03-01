H264 Decoder Python Module
==========================

![Master branch status](https://github.com/DaWelter/h264decoder/actions/workflows/python-package.yml/badge.svg?branch=master)

Note - this fork was modified by Tom Warfel with assistance from Jonathon McCall to provide the following changes:
 1. support for FFMPEG v6+
 2. convert H264 frames into 24-bit BGR images rather than RGB images, to simplify work with OpenCV rather than PILlow/matplotlib
 3. Build on Windows 10/11, Darwin (MacOS), and Linux x86_64 (with plans for ARM64 as well)

This project was originally intended to provide a simple decoder for Python3 scripts to 
decode and display video from a Raspberry Pi camera in H264 format, and is built upon 
the "pybind11" (python bindings for C libraries) tool, and the FFMPEG libraries + include files.
.
While the original code produced RGB decoded files, this version produces BGR decoded files for direct display by OpenCV.

In addition, decoder results now include the numeric frame-type (per libavutil/avutil.h enum AVPictureType.  
Most receently legal values were:
```python
    AV_PICTURE_TYPE_NONE = 0  ///< Undefined
    AV_PICTURE_TYPE_I  = 1    ///< Intra
    AV_PICTURE_TYPE_P  = 2    ///< Predicted
    AV_PICTURE_TYPE_B  = 3    ///< Bi-dir predicted
    AV_PICTURE_TYPE_S  = 4    ///< S(GMC)-VOP MPEG-4
    AV_PICTURE_TYPE_SI = 5    ///< Switching Intra
    AV_PICTURE_TYPE_SP = 6    ///< Switching Predicted
    AV_PICTURE_TYPE_BI = 7    ///< BI type
```




Examples
--------
You can do something like this
```python
import h264decoder
import numpy as np

f = open(thefile, 'rb')
decoder = h264decoder.H264Decoder()
while 1:
  data_in = f.read(1024)
  if not data_in:
    break
  framedatas = decoder.decode(data_in)
  for [frame, width, height, rowsize, pict_type, key_frame] in framedatas:
      image_oversize = np.frombuffer(frame, dtype=np.ubyte, count=len(frame)) 

      # decoded image now returns as BGR rather than RGB
      image = image_oversize.reshape((height, rowsize // 3, 3))
      print("processing video frame at timestamp: ", presentation_time, " height: ", height, " width: ", width )
      if pict_type == 1:
          print("I-frame")
      elif pict_type == 2:
          print("P-frame")
      elif pict_type = 3:
          print("B-frame")
      else:
          print("need to add other cases for this video stream")
      if key_frame == 1:
          print("this image is a key-frame (self-contained and not reliant on previous frames)")

      cv2.imshow("image", image)
      cv2.waitKey(10)
```
There are simple demo programs in the ```examples``` folder. ```display_frames.py``` is probably the one you want to take a look at.

Requirements
------------

* Python 3
* cmake for building
* libav / ffmpeg (swscale, avutil and avcodec)
* pybind11 (will be automatically downloaded from github if not found)


I tested it on

* Ubuntu 22.04, gcc 11, FFMPEG 6.1.1 downloaded and built in WSL2 with Python 3.11 (from sudo apt-get install python3.11-dev)
* Windows 11, Visual Studio Community 2022, with Python 3.12 for windows (from www.python.org/downloads/), and FFMPEG from vcpkg.


Building and Installing
-----------------------

### Windows

The suggested way to obtain ffmpeg is through [vcpkg](https://github.com/microsoft/vcpkg). Assuming all the setup including VC integration has been done, we can install the x64 libraries with

```cmd
vcpkg.exe install ffmpeg:x64-windows
```

We can build the extension module with setuptools almost normally. However cmake is used internally and we have to let it know the search paths to our libs. Hence the additional ```--cmake-args``` argument with the toolchain file as per vcpkg instructions.

```bash
python setup.py build_ext --cmake-args="-DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake"
pip install .
```

Do not use the ```-e``` option (which installs symlinks to the build directory) as it doesn't play nice with the 
Windows filesystem.

----------------------------------------------

Alternatively one can build the extension module manually with cmake.
From the project directory:
```cmd
mkdir [build dir name]
cd [build dir name]
cmake -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake -A x64 ..
cmake --build .
```

### Linux

Should be a matter of installing the libav or ffmpeg libraries. On Debian or Ubuntu:

```bash
sudo apt install libswscale-dev libavcodec-dev libavutil-dev
```

And then running

```bash
pip install .
```

in the project directory.


History
-------
current - modified for BGR output, and to identify key-frames and picture types

### v2

For Python 3. Switch to PyBind11. Module renamed from libh264decoder to h264decoder! Support installation via setuptools.

### v1

For Python 2.7. Depends on Boost Python. Project/Build file generation with CMake.


Credits
-------

* [Michael Welter](https://github.com/DaWelter). Original author.
* [Martin Valgur](https://github.com/valgur).  Switch to pybind11, nice build configuration and more.

License
-------
The code is published under the Mozilla Public License v. 2.0. 
