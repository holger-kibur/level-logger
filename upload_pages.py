import sys
import os
import importlib
import subprocess

PART_FOLDER = "./build/partition_table/"
CONTENT_FILE_PATH = PART_FOLDER + "page_content.part"
CONTENT_TABLE_FILE_PATH = PART_FOLDER + "page_table.part"

idf_path = os.environ["IDF_PATH"]  # get value of IDF_PATH from environment
parttool_dir = os.path.join(idf_path, "components", "partition_table")  # parttool.py lives in $IDF_PATH/components/partition_table

sys.path.append(parttool_dir)  # this enables Python to find parttool module
pt = importlib.import_module("parttool")

# Generate and upload partition table
subprocess.call(["idf.py", "partition-table", "partition-table-flash"], env=os.environ.copy())

# Create part files to store table and content
content_file = open(CONTENT_FILE_PATH, "wb")
table_file = open(CONTENT_TABLE_FILE_PATH, "wb")

# Write two 32 bit fields to the file to use later
table_file.write((0).to_bytes(8, "little"))
table_file.write("\n".encode("UTF-8"))

# Process the page contents
content_files = list(filter(lambda x: x.endswith(".html"), os.listdir("./page_content")))
num_entries = 0
for filename in content_files:
    minified = subprocess.check_output(["minify", f"./page_content/{filename}"])
    table_string = "{:s} {:d} {:d}\n".format(filename.split(".")[0], content_file.tell(), len(minified))
    print(table_string)
    table_file.write(table_string.encode("UTF-8"))
    content_file.write(minified)
    num_entries += 1

# Add table length to the beginning of the table
table_len = table_file.tell() - 9 # Subtract preamble of 9 bytes
table_file.seek(0)
table_file.write(table_len.to_bytes(4, "little"))
table_file.write(num_entries.to_bytes(4, "little"))

# Close partition files 
content_file.close()
table_file.close()

# Write partitions
up_target = pt.ParttoolTarget("/dev/ttyUSB0")
up_target.write_partition(pt.PartitionName("page_table"), CONTENT_TABLE_FILE_PATH)
up_target.write_partition(pt.PartitionName("page_content"), CONTENT_FILE_PATH)

