#!/bin/bash
for i in "$@"
    do
        mv "$i" "$i.temp"
        ffmpeg -i "$i.temp" -vf hflip -c:a copy "$i"
        rm "$i.temp"
    done