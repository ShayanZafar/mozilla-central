#!/bin/sh
# script to update the webvtt library source

URL=https://github.com/rillian/webvtt.git
SRCDIR=src

if [ -d ${SRCDIR}/.git ]; then
  echo "Updating existing checkout..."
  cd ${SRCDIR} && git pull
else
  echo "Downloading source from ${URL}"
  git clone ${URL} ${SRCDIR}
fi
