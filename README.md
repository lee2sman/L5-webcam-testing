# L5-webcam

A little library to access the webcam in L5.

This was last tested on an older version of L5 before I started adding a version number to the library!

# Install

Like normal, your project and the L5.lua library go in your project directory. You also need to add the webcam.lua library and the webcam compiled library, which is `webcam.so` for Linux, or `webcam.dll` for Windows, or `webcam.dylib` for macOS.

```
your-project/
├── main.lua          
├── L5.lua            (L5 library)
├── webcam.lua        (the library module)
├── webcam.c          (C source)
└── webcam.so         (compiled library for Linux)
```

## Next steps

We need to compile webcam.so for different platforms! (webcam.so on Linux, webcam.dll on Windows, webcam.dylib on macOS)

### Compile for Linux

```
gcc -O2 -shared -fPIC -o webcam.so webcam.c -ljpeg
```

### Compile for Windows

```
gcc -O2 -shared -o webcam.dll webcam.c -ljpeg.dll
```

This will require the user has [libjpeg library](https://github.com/winlibs/libjpeg) installed. To make it easier, we could package this by placing in same directory. 

### Compile for macOS

```
clang -O2 -dynamiclib -o webcam.dylib webcam.c -ljpeg
```

Mac users will need libjpeg library. Can be installed `brew install libjpeg` or [manually download](https://mac-dev-env.patrickbougie.com/libjpeg/) and include in same directory.

