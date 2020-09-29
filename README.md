# ExiFREE
    Small EXIF data eraser.

[![Total alerts](https://img.shields.io/lgtm/alerts/g/carloslack/exifree.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/carloslack/exifree/alerts/)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/carloslack/exifree.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/carloslack/exifree/context:cpp)

<p align="left">
    <a href="https://github.com/carloslack/exifree/blob/master/LICENSE"><img alt="Software License" src="https://img.shields.io/badge/MIT-license-green.svg?style=flat-square"></a>
</p>


## About
        Exchangeable Image File Format - EXIF, is a standard that specifies the formats for images,
    sound and ancillary tags used by digital cameras and smartphones.

## Purpose
    To remove (erase) data stored in JPEG files that are part of the EXIF.
    Here are some examples of data that can be stored in image files, depending
    on the configuration of the camera/smartphone:
        - GPS location data;
        - device model and maker;
        - software details;
        - date and time at the moment the picture was taken and more.

## Building
    Update test submodule
        git submodule update --init --recursive .

    Build the binary
        make

    Build and run against the test images imported as git submodule from:
        https://github.com/ianare/exif-samples

        make test

## Usage
    $ ./exifree -h
    Usage: exifree [OPTION]... <FILE>

    Optional arguments
     -f <jpeg file>
     -d <dirname>   output directory
     -v             verbose dump sections
     -r             dry-run
     -s             save exif sections in dirname
     -h             show this help

| Parameter                    | Description                           |
|:----------------------------:|---------------------------------------|
| `<image file>`               | Wipe out exif contents & save the output as "wiped_<image file>" |
| `-r`                         | Dry-run against image file - sanit test without generating an output |
| `-v`                         | Verbose show sections details to standard output |
| `-s`                         | Save sections in output "wiped_<image file>-bin" |
| `-d <destination directory>` | Save output file (and sections if used) in destination directory |
| `-h`                         | Show help and exit |

## Sample output

    Running with -v (long output, showing here begin-end only)

![Screenshot](docs/images/ex1.png)
![Screenshot](docs/images/ex2.png)

    Running with -s

![Screenshot](docs/images/sct1.png)
![Screenshot](docs/images/sct2.png)
![Screenshot](docs/images/sct3.png)

## References
    https://www.awaresystems.be/imaging/tiff/tifftags.html
    http://exif.org

## Bugs
    They are there!
