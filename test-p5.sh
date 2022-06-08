#!/bin/bash
TEST_DIR=/p/course/cs537-swift/tests/p5
INPUT_DIR=${TEST_DIR}/inputs
OUTPUT_DIR=${TEST_DIR}/outputs

#flags
continue_flag=1
test_select=0

#results
test_cnt=0
test_passed=0
check_output=1

function check_p5_files {
	if [ ! -f Makefile ]; then
	    echo "Makefile not found!"
	    exit 1
	fi
}

#imgscan_run disk_image num_jpgs deleted_jpgs longdesc
function imgscan_run {
    test_cnt=$((test_cnt+1))

    # args
    img=${1}
    n_jpgs=${2}
    n_del_jpgs=${3}
    long_desc="Find ${n_jpgs} jpg(s), ${n_del_jpgs} deleted jpg(s) -- ${4}"
    outdir="output_t${test_cnt}"
    executable="runscan"

    echo "[Test${test_cnt} - ${long_desc}]"
	if [ ${test_select} -gt 0 ]; then
	    if [ ${test_select} -ne ${test_cnt} ]; then
			echo "[Skip Test${test_cnt}]"
			echo
			return 0
	    fi
	fi
	echo "  Running \$./${executable} ${img} ${outdir}"

    # Build command string
	run_cmd="./${executable} ${img} ${outdir}"
    run_cmd="timeout 30 ${run_cmd}"

	# Run program
	if ! eval ${run_cmd}; then
		echo "[Test${test_cnt} failed]"
		echo "  Error occured while running ./${executable} ${img} ${outdir}"
		echo
		if [ ${continue_flag} -eq 1 ]; then
			return 1
		fi
		exit 1
	fi

    # compare expected output files to actual ones
    for file in "${OUTPUT_DIR}/${outdir}/"*
    do
        if ! test -f ${outdir}/"${file##*/}"; then
			echo "[Test${test_cnt} failed]"
			echo "  The output file ${file##*/} does not exist."

			if [ ${continue_flag} -eq 1 ]; then
				return 1
			fi
			exit 1
		fi
            
		if ! cmp --silent "${file}" ${outdir}/"${file##*/}"; then
			echo "[Test${test_cnt} failed]"
			echo "  The output is different from the expected output."
			echo "    - expected output path: ${OUTPUT_DIR}/${outdir}/${out_filename}"
			echo "    - actual output path: ${outdir}/${out_filename}"
			echo

			if [ ${continue_flag} -eq 1 ]; then
				return 1
			fi
			exit 1
		fi
    done

    # make sure there are no extra output files in user outdir
    for file in "${outdir}/"*
    do
        if ! test -f "${OUTPUT_DIR}/${outdir}/${file##*/}"; then
			echo "[Test${test_cnt} failed]"
			echo "  You created some extra file ${file##*/}"

			if [ ${continue_flag} -eq 1 ]; then
				return 1
			fi
			exit 1
		fi
    done    

    echo "[Test${test_cnt} Success]"
    echo ""
	test_passed=$((test_passed+1))
}
   

# ******************************************************************************
# Main Testing
# ******************************************************************************

# mode select
while [[ $# -gt 0 ]]
do
key="$1"
case $key in
    -t)
	test_select=$2
	shift
	shift
    ;;
    -c)
	continue_flag=1
	shift
    ;;
esac
done

executable = "runscan"

# check Makefile exists
if [ ! -f Makefile ]; then
    echo "Makefile not found!"
    exit 1
fi


echo "Deleting old output directories..."
rm -rf output_*

# make project
rm -f ${executable}
make

# check executable exists
if [ ! -f $executable ]; then
    echo "*** ERROR: ${executable} not built by make command"
    exit
fi

#imgscan_run disk_image num_jpgs deleted_jpgs longdesc
imgscan_run "${INPUT_DIR}/img1.in" 1 0 "one jpg in root dir"
imgscan_run "${INPUT_DIR}/img2.in" 0 1 "one deleted jpg in root dir"
imgscan_run "${INPUT_DIR}/img3.in" 4 0 "multiple jpgs in root dir"
imgscan_run "${INPUT_DIR}/img4.in" 1 4 "multiple jpgs in root dir, mixed"
imgscan_run "${INPUT_DIR}/img5.in" 2 0 "jpgs in subdir"
imgscan_run "${INPUT_DIR}/img6.in" 4 0 "jpgs of mixed size"
imgscan_run "${INPUT_DIR}/img7.in" 4 0 "standard jpgs with variable length names in many directories"
imgscan_run "${INPUT_DIR}/img8.in" 0 4 "deleted jpgs with variable length names"
imgscan_run "${INPUT_DIR}/img9.in" 1 0 "huge jpg"
imgscan_run "${INPUT_DIR}/img10.in" 100 0 "jpg and txt files"


# *** RESULTS ***
echo ""
echo "Test Result [${test_passed}/${test_cnt}]"

exit 0
