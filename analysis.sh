#!/bin/bash
FILES=`zenity --file-selection --title="Select video files for analysis" --multiple --file-filter='Video files (mp4, avi) | *.mp4 *.avi'`
zenity --question --text="Flip videos horizontally?"
FLIP_HORIZ=$?
echo "$FLIP_HORIZ"
IS_CALIBRATED=0
NETFILE="/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/multiphase.onnx"
CALIB_VALUE=""
while IFS="|" read -ra FILES; do
    for i in "${FILES[@]}"; do
        if [ $FLIP_HORIZ -eq 0]
        then
            mv "$i" "$i.temp"
            ffmpeg -i "$i.temp" -vf hflip -c:a copy "$i"
            rm "$i.temp"
        fi
        if [ $IS_CALIBRATED -eq 0 ]
        then
            "/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/Release/MpembaVideoAnalysis" "$i" "$NETFILE" 1000
            CALIB_VALUE=`zenity  --title  "Calibration Value" --entry --text "Enter calibration value in scientific notation"`
            echo "$CALIB_VALUE"
            IS_CALIBRATED=1
        else
            "/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/Release/MpembaVideoAnalysis" "$i" "$NETFILE" --calib-value=$CALIB_VALUE 1000
        fi
    done
done <<< "$FILES"
