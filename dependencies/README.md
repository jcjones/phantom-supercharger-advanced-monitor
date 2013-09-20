Dependencies
============

Each dependency must be loaded into the Arduino IDE by selecting Libraries -> Add Library

* RTClib as a git submodule at commit 573581794b73dc70bccc659df9d54a9f599f4260
* u8glib v1.13 (Arduino Environment) as a forked sudmobule at commit 32689c23d41eebf70e3341bc5e18ea4d4fa82d86 , or downloaded from [https://code.google.com/p/u8glib/downloads/detail?name=u8glib_arduino_v1.13.zip]
* Adafruit SD library as a git submodule at commit 3216c04075860edece362818c9c413e9c061a186
* OneWire as a git submodule at commit 85d1aae63ea4919c64151e03f7e24c2efbc40198

Notes on the U8glib
-------------------
If you choose to use the git submodule, you will need to produce an "arduino" release; do that by running 
> dependencies/u8glib/tools/release/arduino/create_release.sh
and then importing the resulting U8glib folder at dependencies/u8glib/tools/release/arduino/libaries into the IDE.
