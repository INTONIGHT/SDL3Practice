Instructions for building this in the future
In the properties of the project in the C++ part general link it to the include part of that: \SDL3-devel-3.2.18-VC\SDL3-3.2.18\include
should be what you want there In linker additional input add SDL3.lib in additional directories in the general tab link it to: SDL3-devel-3.2.18-VC\SDL3-3.2.18\lib\x86
that x part has to correspond to whatever you have in your debug next to it. Make sure to build and clean build frequently. From SDL3-devel-3.2.18-VC\SDL3-3.2.18\lib\x86 copy the SDL3.dll
and paste into the directory:SDL3Practice\SDL3Practice\SDL3Practice or wherver it works in this case i know it worked when the error was just a file not found.
For this project make sure you add the image to the same directory with the same network.
For images: use SDL Image and download for Visual studio (vc)
Follow steps for the SDL and the main difference for the linker inpout is it is SDL3_image.lib and then link it to the lib folder. same thing for downloading the .dll into the project
