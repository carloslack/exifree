# ExiFREE
    Small EXIF data eraser.

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
    $ ./exifree
    Usage: exifree [OPTION]... <FILE>

    Optional arguments
     -f <jpeg file>
     -d <dirname>   output directory
     -v             verbose dump sections
     -r             dry-run
     -s             save exif sections in dirname

## References
    https://www.awaresystems.be/imaging/tiff/tifftags.html
    http://exif.org

## Bugs
    They are there!
