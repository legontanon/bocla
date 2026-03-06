# CMake script to generate FM sine table
# This is called during build and generates fm_sine_quarter.inc

find_program(PYTHON3_EXECUTABLE python3)
if(NOT PYTHON3_EXECUTABLE)
    find_program(PYTHON3_EXECUTABLE python)
endif()

if(NOT PYTHON3_EXECUTABLE)
    message(FATAL_ERROR "Python3 not found. Install Python 3 to generate sine tables.")
endif()

# Get the script location from the source directory
set(SCRIPT_DIR "${CMAKE_CURRENT_LIST_DIR}")
if(NOT EXISTS "${SCRIPT_DIR}/gen_fm_sine_table.py")
    message(FATAL_ERROR "gen_fm_sine_table.py not found in source directory")
endif()

# Run the Python script to generate the sine table
# Pass source directory so it can read FM_SINE_QUARTER_SIZE from cfg.h
execute_process(
    COMMAND "${PYTHON3_EXECUTABLE}" "${SCRIPT_DIR}/gen_fm_sine_table.py" "fm_sine_quarter.inc" "${SCRIPT_DIR}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    RESULT_VARIABLE PYTHON_RESULT
    OUTPUT_VARIABLE PYTHON_OUTPUT
    ERROR_VARIABLE PYTHON_ERROR
)

if(NOT PYTHON_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate sine table:\n${PYTHON_ERROR}")
endif()

message(STATUS "${PYTHON_OUTPUT}")
