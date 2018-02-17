#!/bin/bash

# list all the test files
FILES=$(ls | grep _test.c)
# template file name
TEMPLATE=template.c
# output test file name
OUTPUT=test.c

# clear the output file 
echo "" > $OUTPUT

# write all of the includes for the test file
for i in $FILES; do
    echo '#include "'$i'"' >> $OUTPUT
done

# read the lines from the template
while IFS= read -r line
do 
    # see if we found the line where we need to fill
    # in the calls to test functions
    match=$(echo $line | grep INSERT)
    length=$(echo $match | wc -m)
    # if the length of the match was greater than one
    # then we need to write the functions into the file
    if [[ $((length)) > 1 ]]; then 
        # write a function for each test file
        for i in $FILES; do
            echo "    run_${i/\.c/()};" >> $OUTPUT
        done
    else 
        # write the line from the template
        echo "$line" >> $OUTPUT
    fi
done < "$TEMPLATE"

# for some reason we are missing this bracket at the end,
# so we need to append it
echo } >> $OUTPUT
