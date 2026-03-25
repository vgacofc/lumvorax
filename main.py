import ctypes
import os

_lib_paths = [
    "/nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.2.1.20250322-lib/lib/libstdc++.so.6",
    "/lib/x86_64-linux-gnu/libstdc++.so.6",
    "/usr/lib/x86_64-linux-gnu/libstdc++.so.6",
]
for _p in _lib_paths:
    if os.path.exists(_p):
        ctypes.CDLL(_p)
        break


def main():
    print("Hello from repl-nix-workspace!")


if __name__ == "__main__":
    main()
