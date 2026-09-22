#! /bin/sh

# This code is mostly by Kyle Rosenberg augmenting code by
# Prof. Neil Kirby. I cleaned it up with a few minor additions.
# -Adam C. Champion

# Your name, which is inserted at the top of each header file
# YOU MUST CHANGE THIS TO YOUR NAME BEFORE USING THIS FILE
# Change the next line to have your name.
# This comment will be at the top of every header file.
readonly AUTHOR_NAME="Vy Nguyen"
readonly DATE=`date +%Y`
readonly COPYRIGHT="Copyright (C) $DATE $AUTHOR_NAME"

# do the mystery that the professors did for us
# This uses ctags to get function names that are visible outside the file
# it uses awk to collect the symbols according to what file they live in.
# In this usage, .vs means "visible symbols"
echo "Creating header files..."
ctags -x --c-kinds=f --file-scope=no *.c | tr -d '{}' | awk -f headers.awk

# create the header files from the visual symbols the professors did for us
for f in *.vs; do
    [ -f "$f" ] || break

    # get the name of the file with the .h extension
    filename="$(basename $f .vs).h"

    # notify the user we are crafting them a delicious header file
    echo -e "\tCreating $filename from $f"

    # Add the typedef for int buffer_item in buffer.h (only)
    if [[ $filename == 'buffer.h' ]]; then
        echo -e "/* $COPYRIGHT */\n" > $filename
        echo -e 'typedef int buffer_item;\n' >> $filename
    else
        # Clear the file and write the aution comment string
        echo -e "/* $COPYRIGHT */\n" > $filename
    fi

    # Remove the followng line or replace it with any additional text you
    # want in every header file.
    echo -e "/* Auto-generated */\n" >> $filename

    # Append everything in the visual symbol files (function declarations)
    cat $f >> $filename
done

date > headers_timestamp
echo "Done creating headers."
