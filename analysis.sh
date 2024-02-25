#!/bin/bash
FILES=`zenity --file-selection --title="Select video files for analysis" --multiple --file-filter='Video files (mp4, avi) | *.mp4 *.avi'`
IS_CALIBRATED=0
NETFILE="/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/multiphase.onnx"
CALIB_VALUE=""
while IFS="|" read -ra FILES; do
    for i in "${FILES[@]}"; do
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
