#!/bin/bash

echo "Make app.png image."
montage win1.png win2.png -geometry 600x+8+8 -frame 6 app.png
