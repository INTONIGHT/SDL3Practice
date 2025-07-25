Instructions for building this in the future
In the properties of the project in the C++ part general link it to the include part of that: \SDL3-devel-3.2.18-VC\SDL3-3.2.18\include
should be what you want there In linker additional input add SDL3.lib in additional directories in the general tab link it to: SDL3-devel-3.2.18-VC\SDL3-3.2.18\lib\x86
that x part has to correspond to whatever you have in your debug next to it. Make sure to build and clean build frequently. From SDL3-devel-3.2.18-VC\SDL3-3.2.18\lib\x86 copy the SDL3.dll
and paste into the directory: SDL3Practice\SDL3Practice\SDL3Practice or wherever it works in this case i know it worked when the error was just a file not found.
For this project make sure you add the image to the same directory with the same network.
For images: use SDL Image and download for Visual studio (vc)
Follow steps for the SDL and the main difference for the linker inpout is it is SDL3_image.lib and then link it to the lib folder. same thing for downloading the .dll into the project
For GLM https://github.com/g-truc/glm/releases/tag/1.0.1 download the zip folder then in your project settings for include set the link to include glm-1.0.1-light im not sure if it needs to be glm
in it i lost my mind spending way too much time to figure that you can just include it without the cmake nonsense just follow steps for adding the sdl3 part.
This needs C++ version 20 but you can adjust that in project properties in C++ in language and then change the version to 20 or latest version
