#!/bin/bash

if [ $? == 0 ]; then
  ino upload
  while [ $? != 0 ]; do
    echo "Upload failed, trying again"
    ino upload
  done
fi
