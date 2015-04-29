# About
This program reads binary data from specified process memory and writes it to stdout.

# Platforms
This programs runs on Linux operating system.

For Windows OS, I made similar program in the past.
http://en.sourceforge.jp/projects/dataviewer/

## How to build
```bash
$ g++ main.cpp
```
## Usage
While target process is running...
```bash
$ ./a.out pid_or_process_name 0xb5b16008 1000000 | display -size 500x500 -depth 8 BGRA:-
```

I'm very reluctant to rewrite versatile image display routines so I delegate ImageMagick to do the job.

