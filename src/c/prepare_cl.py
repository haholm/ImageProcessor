import re
import os
import argparse
import sys

CL_PLACEHOLDER = "static const char *cl_string;"
CL_PLACEHOLDER_PATTERN = re.escape(CL_PLACEHOLDER[:-1]) + r"\s*=?(\s*\".*\"\s*)*;"

def prepare_cl(src: str, dest: str):
    with open(src, "r") as f:
        cl_code = f.read().split(os.linesep)
        
    cl_code_string = ""
    for cl_line in cl_code:
        cl_line = re.sub("\"", r"\\\"", cl_line)
        cl_code_string += f"\"{cl_line}\\\\n\"" + os.linesep

    with open(dest, "r") as f:
        header_code = f.read()
        
    if (not re.findall(CL_PLACEHOLDER_PATTERN, header_code)):
        print(f"Error: Could not find cl_string placeholder \"{CL_PLACEHOLDER}\" in dest.")
        sys.exit(-1)
        
    with open(dest, "w") as f:
        f.write(re.sub(CL_PLACEHOLDER_PATTERN, f"{CL_PLACEHOLDER[:-1]} = {cl_code_string};", header_code))
            
if __name__ == "__main__":
    parser = argparse.ArgumentParser(
                prog=os.path.basename(__file__),
                description=f'Prepares CL code for use in C header constant. Expects placeholder \"{CL_PLACEHOLDER}\" is in dest.')
    parser.add_argument("src")
    parser.add_argument("dest")
    args = parser.parse_args()
    prepare_cl(args.src, args.dest)