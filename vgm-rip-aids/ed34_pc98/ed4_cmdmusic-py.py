from subprocess import call

def execute(data_id, base_id):
	name_buf = []
	
	# starting number, ensure all 4 files in groups of 4 from base
	for i in xrange(4):
		datadir  = "{0:02d}".format(data_id)
		datablob = "{0:03d}".format(base_id+i)
		
		try:
			f = open("DATA{0}\\{1}".format(datadir, datablob), "r")
		except IOError:
			print "Epic fail.  You can't count."
			return
		
		name_buf.append(datablob)
		print "DEBUG: {1} opened {0}".format(datablob, i)
   
	for i in xrange(4):
		print "copying datablob {0} to DATA10\\{1:03d}...".format(name_buf[i], 4+i)
		call(["copy", "DATA{0}\\{1}".format(datadir, name_buf[i]), "DATA10\\{0:03d}".format((4+i))], shell=True)
	
	call("cd DATA10 && packme.bat", shell=True)
	print "OPN: {0}_{1:03d}, OPNA: {0}_{2:03d}".format(datadir, base_id, base_id+2)
	return
	
if __name__ == "__main__":
	import argparse
	parser = argparse.ArgumentParser(description='Copy files and call stuff?')
	parser.add_argument('data_id', type=int, help='Data Directory ID')
	parser.add_argument('base_id', type=int, help='Base Data ID')
	args = parser.parse_args()
	
	execute(args.data_id, args.base_id)
else:
	print "You make the call yourself."