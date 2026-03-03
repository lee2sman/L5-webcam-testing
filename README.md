# L5-webcam

A little library to access the webcam in L5.

# Install

Like normal, your project and L5 go in a project directory. You also need to add the webcam.lua library and the webcam compiled library, which is `webcam.so` for Linux, or `webcam.dll` for Windows, or `webcam.dylib` for macOS.

```
your-project/
├── main.lua          
├── L5.lua            (L5 library)
├── webcam.lua        (the library module)
├── webcam.c          (C source)
└── webcam.so         (compiled library for Linux)
```
