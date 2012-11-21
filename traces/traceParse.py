#!/usr/bin/python

import re, os 
import string
import sys
import array

if len(sys.argv) != 2:
  sys.exit("Must specify trace file (.gz)")


gztrace_filename = sys.argv[1]

if gztrace_filename.endswith(".gz") == True:
	tracefile_filename = sys.argv[1][0:len(sys.argv[1])-3]
else:
	tracefile_filename = gztrace_filename


if tracefile_filename.endswith(".trc") == False:
	tracefile_filename = tracefile_filename + ".trc"
	
if tracefile_filename.startswith("gz") == True:
	tracefile_filename = "pin_" + tracefile_filename
	
if tracefile_filename.startswith("DG") == True:
	tracefile_filename = "DGpin_" + tracefile_filename

if tracefile_filename.startswith("hmtt") == True:
	tracefile_filename = "pin_" + tracefile_filename
	temp_trace = gztrace_filename
else:
	temp_trace = tracefile_filename + ".temp"

	zcat_cmd = "zcat";
	# accomodate OSX
	if os.uname()[0] == "Darwin":
		print "Detected OSX, using gzcat..."
		zcat_cmd = "gzcat"

	if not os.path.exists(gztrace_filename):
	  print "Could not find gzipped tracefile either"
	  quit()
	else:
	  print "Unzipping gz trace...",
	  os.system("%s %s > %s" % (zcat_cmd, gztrace_filename, temp_trace))
	  print "OK"
  

print "Parsing ",
outfile = open(tracefile_filename,"w")
if not os.path.exists(tracefile_filename):
  print "FAILED"
  quit()
tracefile = open(temp_trace,"r")

if tracefile_filename.startswith("k6"):
  print "k6 trace ...",
  linePattern = re.compile(r'(0x[0-9A-F]+)\s+([A-Z_]+)\s+([0-9.,]+)\s+(.*)')
  for line in tracefile:
    searchResult = linePattern.search(line)
    if searchResult:
        (address,command,time,units) = searchResult.groups()

        length = len(time)
        time = time[0:length-5]
        temp = len(time)
        if temp==0:
            time = "0"
        time = string.replace(time,",","")
        time = string.replace(time,".","")
        if command != "BOFF" and command != "P_INT_ACK":
            outfile.write("%s %s %s\n" % (address,command,time))
  print "OK"
            
elif tracefile_filename.startswith("k7"):
  print "k7 trace ...",
  linePattern = re.compile(r'(0x[0-9A-F]+)\s+([A-Z_]+)\s+([0-9.,]+)\s+(.*)')
  for line in tracefile:
    searchResult = linePattern.search(line)
    if searchResult:
        (address,command,time,units) = searchResult.groups()

        length = len(time)
        time = time[0:length-5]
        temp = len(time)
        if temp==0:
            time = "0"
        time = string.replace(time,",","")
        time = string.replace(time,".","")
        if command == "P_MEM_WR":
        	outfile.write("%s %s %s 8 %s%s%s%s%s%s%s%s\n" % (address,command,time,time,time,time,time,time,time,time,time)) 
        elif command != "BOFF" and command != "P_INT_ACK":
            outfile.write("%s %s %s\n" % (address,command,time))
  print "OK"
  
elif tracefile_filename.startswith("pin_hmtt"):
  print "pin_hmtt trace ...",
  stime = 0
  linePattern = re.compile(r'\s*([0-9]+)\s+([01])\s+([0-9a-f]+)\s+.*')
  for line in tracefile:
    searchResult = linePattern.search(line)
    if searchResult:
        (time,command,address) = searchResult.groups()
        
        while len(address) < 10: 
            address = "0" + address
        if command=="0":
        	outfile.write("0x%s P_MEM_WR %s\n" % (address,time)) 
        elif command=="1":
            outfile.write("0x%s P_MEM_RD %s\n" % (address,time))
  print "OK"

elif tracefile_filename.startswith("pin"):
  print "pin trace ...",
  stime = 0
  linePattern = re.compile(r'\s*([0-9]+)\s+([0-9a-f]+)\s+([01])\s+.*')
  for line in tracefile:
    searchResult = linePattern.search(line)
    if searchResult:
        (time,address,command) = searchResult.groups()
        
        while len(address) < 10: 
            address = "0" + address
        stime = stime + string.atoi(time)
        if command=="0":
        	outfile.write("0x%s P_MEM_WR %s\n" % (address,stime)) 
        elif command=="1":
            outfile.write("0x%s P_MEM_RD %s\n" % (address,stime))
  print "OK"
            
elif tracefile_filename.startswith("DGpin"):
  print "DGpin trace ...",
  stime = 0
  linePattern = re.compile(r'\s*([0-9]+)\s+([0-9a-f]+)\s+([01])\s+([1-8])\s+.*')
  for line in tracefile:
    searchResult = linePattern.search(line)
    if searchResult:
        (time,address,command,units) = searchResult.groups()
        
        while len(address) < 10: 
            address = "0" + address
        stime = stime + string.atoi(time)
        if command=="0":
        	outfile.write("0x%s P_MEM_WR %s %s %s%s%s%s\n" % (address,stime,units,stime,address,stime,address)) 
        elif command=="1":
            outfile.write("0x%s P_MEM_RD %s %s\n" % (address,stime,units))
  print "OK"
      
else:
  print "Unknown trace file!!!"
  quit()

os.system("rm %s" % temp_trace);
