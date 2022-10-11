import sys
import os
import importlib
import subprocess

PART_FOLDER = "./build/partition_table/"
CONTENT_FILE_PATH = PART_FOLDER + "page_table.part"
CONTENT_TABLE_FILE_PATH = PART_FOLDER + "page_content.part"

idf_path = os.environ["IDF_PATH"]  # get value of IDF_PATH from environment
parttool_dir = os.path.join(idf_path, "components", "partition_table")  # parttool.py lives in $IDF_PATH/components/partition_table

sys.path.append(parttool_dir)  # this enables Python to find parttool module
pt = importlib.import_module("parttool")

# Generate and upload partition table
subprocess.call(["idf.py", "partition-table", "partition-table-flash"], env=os.environ.copy())

# Create part files to store table and content
content_file = open(CONTENT_FILE_PATH, "wb")
table_file = open(CONTENT_TABLE_FILE_PATH, "wb")

# Process the page contents
content_files = os.listdir("./page_content")
for filename in content_files:
    if not filename.endswith(".html"):
        # Do not write generated .part files
        continue
    minified = subprocess.check_output(["minify", f"./page_content/{filename}"])
    table_string = "{:s} {:d} {:d} ".format(filename.split(".")[0], content_file.tell(), len(minified))
    print(table_string)
    table_file.write(table_string.encode("UTF-8"))
    content_file.write(minified)

# Close partition files 
content_file.close()
table_file.close()

# Write partitions
up_target = pt.ParttoolTarget("/dev/ttyUSB0")
up_target.write_partition(pt.PartitionName("page_table"), CONTENT_TABLE_FILE_PATH)
up_target.write_partition(pt.PartitionName("page_content"), CONTENT_FILE_PATH)

