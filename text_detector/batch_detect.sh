DIR=$1
EXT=$2
echo "Directory: $1"
echo "Process files in the directory with extension: $2"

# Iterate files in $DIR with extension $EXT
for FILE in "$DIR"/*."$EXT"; do
    # Check if the file exists.
    if test -f "$FILE"; then
        # Check if the file is already processed
        # by checking if a JSON file of the same basename exists.
        JSON_FILE="${FILE%.*}.json"
        if test -f $JSON_FILE; then
            echo "File: $FILE"
            echo "Skip it because a corresponding JSON file exists."
            continue
        fi

        # Detect text in the file.
        echo "Process $FILE"
        node detect.js "$FILE"
    fi
done
