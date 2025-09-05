# Compiles an .asm file into a .gb 
# Usage: python3 compile.py yourfile.asm
# Generates yourfile.o yourfile.sym yourfile.gb
# Also removes old *.o, *.sym, *.gb
import sys
import subprocess
import glob
import os

filename = sys.argv[1]
no_asm = filename.removesuffix(".asm")

rm = glob.glob("*.o")
rm.extend(glob.glob("*.gb"))
rm.extend(glob.glob("*.sym"))
for f in rm:
    os.remove(f)
command = f"rgbasm {filename} -o {no_asm}.o && rgblink -n {no_asm}.sym -o {no_asm}.gb {no_asm}.o && rgbfix -v -p 0xFF {no_asm}.gb"

subprocess.run(command, shell=True)
