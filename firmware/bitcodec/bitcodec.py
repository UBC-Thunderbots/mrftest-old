import os.path
import sys

def makefnprefix(structname):
	structname = structname[0].lower() + structname[1:]
	result = ""
	for ch in structname:
		if ch.isupper():
			result += "_" + ch.lower()
		else:
			result += ch
	return result

class DefFileError(Exception):
	def __init__(self, value):
		self.value = value;

	def __str__(self):
		return repr(self.value)

class Element(object):
	def name(self):
		raise NotImplementedError

	def type(self):
		raise NotImplementedError

	def utype(self):
		raise NotImplementedError

	def offset(self):
		raise NotImplementedError

	def length(self):
		raise NotImplementedError

	def default(self):
		raise NotImplementedError

	def uval(self, path):
		raise NotImplementedError

	def spuriousbits(self):
		raise NotImplementedError

	def genpreamble(self, cfile):
		pass

	def postdecodefn(self):
		return ""

class UnsignedElement(Element):
	def __init__(self, params):
		if len(params) != 5:
			raise DefFileError("BITCODEC_DATA_U requires five parameters!")
		self._params = params

	def name(self):
		return self._params[1]

	def type(self):
		return self._params[0]

	def utype(self):
		return self.type()

	def offset(self):
		return int(self._params[2])

	def length(self):
		return int(self._params[3])

	def default(self):
		return self._params[4]

	def uval(self, path):
		return path + self.name()

	def spuriousbits(self):
		return False

class SignedElement(Element):
	def __init__(self, params):
		if len(params) != 6:
			raise DefFileError("BITCODEC_DATA_S requires six parameters!")
		self._params = params

	def name(self):
		return self._params[2]

	def type(self):
		return self._params[0]

	def utype(self):
		return self._params[1]

	def offset(self):
		return int(self._params[3])

	def length(self):
		return int(self._params[4])

	def default(self):
		return self._params[5]

	def uval(self, path):
		return "(({}) {})".format(self.utype(), path + self.name())

	def spuriousbits(self):
		return True

	def genpreamble(self, cfile):
		cfile.write("static inline {} {}_to_signed({} x) {{\n".format(self.type(), self.name(), self.utype()))
		cfile.write("\tif (x & ({}) UINTMAX_C(0x{:X})) {{\n".format(self.utype(), 1 << (self.length() - 1)))
		cfile.write("\t\treturn ({}) ({}) (x | ({}) UINTMAX_C(0x{:X}));\n".format(self.type(), self.utype(), self.utype(), 0xFFFFFFFF & ~((1 << self.length()) - 1)))
		cfile.write("\t} else {\n")
		cfile.write("\t\treturn ({}) x;\n".format(self.type()))
		cfile.write("\t}\n")
		cfile.write("}\n")
		cfile.write("\n")

	def postdecodefn(self):
		return "{}_to_signed".format(self.name())

class BoolElement(Element):
	def __init__(self, params):
		if len(params) != 3:
			raise DefFileError("BITCODEC_DATA_BOOL requires three parameters!")
		self._params = params

	def name(self):
		return self._params[0]

	def type(self):
		return "BOOL"

	def utype(self):
		return "uint8_t"

	def offset(self):
		return int(self._params[1])

	def length(self):
		return 1

	def default(self):
		return self._params[2]

	def uval(self, path):
		return path + self.name()

	def spuriousbits(self):
		return False

def run():
	# Check command-line parameters.
	if not 3 <= len(sys.argv) <= 4:
		print("Usage")
		print(sys.argv[0] + " structname deffile [outbase]")
		sys.exit(1)

	# Extract command-line arguments.
	structname = sys.argv[1]
	fnprefix = makefnprefix(structname)
	deffilename = sys.argv[2]
	if len(sys.argv) >= 4:
		outbasename = sys.argv[3]
	else:
		outbasename = os.path.splitext(deffilename)[0]
	cfilename = outbasename + ".c"
	hfilename = outbasename + ".h"

	# Load the structure definition.
	fields = []
	with open(deffilename, "rt") as deffile:
		for line in deffile:
			line = line.strip()
			if not line.startswith("//") and len(line) != 0:
				if line.count("(") != 1 or line.count(")") != 1:
					raise DefFileError("Every non-blank, non-comment line must contain exactly one paren pair!")
				if line.find(")") != len(line) - 1:
					raise DefFileError("Closing paren must be last character on a line!")
				(command, params) = line[0:-1].split("(")
				params = [p.strip() for p in params.split(",")]
				if command == "BITCODEC_DATA_U":
					fields.append(UnsignedElement(params))
				elif command == "BITCODEC_DATA_S":
					fields.append(SignedElement(params))
				elif command == "BITCODEC_DATA_BOOL":
					fields.append(BoolElement(params))
				else:
					raise DefFileError("Unrecognized command \"" + command + "\"")

	# Compute the size of a structure in bytes.
	structbytes = (fields[-1].offset() + fields[-1].length() + 7) // 8
	
	# Generate output.
	try:
		with open(cfilename, "wt") as cfile, open(hfilename, "wt") as hfile:
			hfile.write("/* This file was generated from {}. Do not edit it directly. */\n".format(deffilename))
			hfile.write("\n")
			include_guard = outbasename.upper().replace("../", "").replace("/", "_")
			hfile.write("#ifndef {}_H\n".format(include_guard))
			hfile.write("#define {}_H\n".format(include_guard))
			hfile.write("\n")
			hfile.write("#include <stdbool.h>\n")
			hfile.write("#include <stdint.h>\n")
			hfile.write("\n")
			hfile.write("/**\n")
			hfile.write(" * \\brief The unpacked representation of a {}\n".format(structname))
			hfile.write(" */\n")
			hfile.write("typedef struct {\n")
			for f in fields:
				hfile.write("\t{} {};\n".format(f.type(), f.name()))
			hfile.write("} " + structname + "_t;\n")
			hfile.write("\n")
			hfile.write("enum {\n")
			hfile.write("\t/**\n")
			hfile.write("\t * \\brief The number of bytes in the packed representation of a {}.\n".format(structname))
			hfile.write("\t */\n")
			hfile.write("\t{}_BYTES = {}\n".format(fnprefix.upper(), structbytes))
			hfile.write("};\n")
			hfile.write("\n")
			hfile.write("/**\n")
			hfile.write(" * \\brief Initializes a {} to its default values.\n".format(structname))
			hfile.write(" *\n")
			hfile.write(" * \\param[out] obj the object to initialize.\n")
			hfile.write(" */\n")
			hfile.write("void {}_init(__data {}_t *obj);\n".format(fnprefix, structname))
			hfile.write("\n")
			hfile.write("/**\n")
			hfile.write(" * \\brief Packs a {} into its packed binary representation.\n".format(structname))
			hfile.write(" *\n")
			hfile.write(" * \\param[out] buffer the buffer into which to store the packed data.\n")
			hfile.write(" *\n")
			hfile.write(" * \\param[in] obj the object to encode.\n")
			hfile.write(" */\n")
			hfile.write("void {}_encode(__data void *buffer, __data const {}_t *obj);\n".format(fnprefix, structname))
			hfile.write("\n")
			hfile.write("/**\n")
			hfile.write(" * \\brief Unpacks a {} from its packed binary representation.\n".format(structname))
			hfile.write(" *\n")
			hfile.write(" * \\param[out] obj the object into which to store the decoded data.\n")
			hfile.write(" *\n")
			hfile.write(" * \\param[in] buffer the packed data to decode.\n")
			hfile.write(" */\n")
			hfile.write("void {}_decode(__data {}_t *obj, __data const void *buffer);\n".format(fnprefix, structname))
			hfile.write("\n")
			hfile.write("#endif\n")
			hfile.write("\n")

			cfile.write("/* This file was generated from {}. Do not edit it directly. */\n".format(deffilename))
			cfile.write("\n")
			cfile.write('#include "{}"\n'.format(hfilename))
			cfile.write("\n")
			for f in fields:
				f.genpreamble(cfile)
			cfile.write("void {}_init(__data {}_t *obj) {{\n".format(fnprefix, structname))
			for f in fields:
				cfile.write("\tobj->{} = {};\n".format(f.name(), f.default()))
			cfile.write("}\n")
			cfile.write("\n")
			cfile.write("void {}_encode(__data void *buffer, __data const {}_t *obj) {{\n".format(fnprefix, structname))
			cfile.write("\t__data uint8_t *p = buffer;\n")
			cfile.write("\n")
			for i in range(0, structbytes):
				cfile.write("\tp[{}] = (uint8_t) (".format(i))
				found = False
				for f in fields:
					if (8 * i < f.offset() + f.length()) and (8 * i + 7 >= f.offset()):
						if found:
							cfile.write(" | ")
						found = True
						bits_before_this_byte = max(0, 8 * i - f.offset())
						bits_this_byte_before = max(0, f.offset() - 8 * i)
						bits_this_byte = min(8 - bits_this_byte_before, f.length() - bits_before_this_byte)
						bits_after_this_byte = f.length() - bits_before_this_byte - bits_this_byte
						bits_this_byte_after = 8 - bits_this_byte_before - bits_this_byte
						shift_distance = bits_after_this_byte - bits_this_byte_after
						mask = ((1 << (bits_this_byte + bits_this_byte_after)) - 1) & ~((1 << bits_this_byte_after) - 1)
						if shift_distance > 0:
							if mask == 0xFF or (not f.spuriousbits() and bits_before_this_byte == 0):
								cfile.write("((uint8_t) ({} >> {}))".format(f.uval("obj->"), shift_distance))
							else:
								cfile.write("((uint8_t) (((uint8_t) ({} >> {})) & (uint8_t) 0x{:02X}))".format(f.uval("obj->"), shift_distance, mask))
						elif shift_distance < 0:
							if mask & 0x80 or not f.spuriousbits():
								cfile.write("((uint8_t) (((uint8_t) {}) << {}))".format(f.uval("obj->"), -shift_distance))
							else:
								cfile.write("((uint8_t) ((((uint8_t) {}) << {}) & ((uint8_t) 0x{:02X})))".format(f.uval("obj->"), -shift_distance, mask))
						else:
							if mask & 0x80 or not f.spuriousbits():
								assert mask == 0xFF or not f.spuriousbits()
								cfile.write("((uint8_t) {})".format(f.uval("obj->")))
							else:
								cfile.write("(((uint8_t) {}) & ((uint8_t) 0x{:02X}))".format(f.uval("obj->"), mask))
				if not found:
					cfile.write("0")
				cfile.write(");\n")
			cfile.write("}\n")
			cfile.write("\n")
			cfile.write("void {}_decode(__data {}_t *obj, __data const void *buffer) {{\n".format(fnprefix, structname))
			cfile.write("\t__data const uint8_t *p = buffer;\n")
			cfile.write("\n")
			for f in fields:
				cfile.write("\tobj->{} = {}(({}) (".format(f.name(), f.postdecodefn(), f.utype()))
				first = True
				for i in range(0, structbytes):
					if (8 * i < f.offset() + f.length()) and (8 * i + 7 >= f.offset()):
						if not first:
							cfile.write(" | ")
						first = False
						bits_before_this_byte = max(0, 8 * i - f.offset())
						bits_this_byte_before = max(0, f.offset() - 8 * i)
						bits_this_byte = min(8 - bits_this_byte_before, f.length() - bits_before_this_byte)
						bits_after_this_byte = f.length() - bits_before_this_byte - bits_this_byte
						bits_this_byte_after = 8 - bits_this_byte_before - bits_this_byte
						shift_distance = bits_after_this_byte - bits_this_byte_after
						mask = ((1 << (bits_this_byte + bits_this_byte_after)) - 1) & ~((1 << bits_this_byte_after) - 1)
						if shift_distance > 0:
							if mask == 0xFF:
								cfile.write("(({}) ((({}) p[{}]) << {}))".format(f.utype(), f.utype(), i, shift_distance))
							else:
								cfile.write("(({}) ((({}) (p[{}] & (uint8_t) 0x{:02X})) << {}))".format(f.utype(), f.utype(), i, mask, shift_distance))
						elif shift_distance < 0:
							if mask & 0x80:
								cfile.write("((uint8_t) (p[{}] >> {}))".format(i, -shift_distance))
							else:
								cfile.write("((uint8_t) (((uint8_t) (p[{}] & (uint8_t) 0x{:02X})) >> {}))".format(i, mask, -shift_distance))
						else:
							if mask & 0x80:
								assert mask == 0xFF
								cfile.write("p[{}]".format(i))
							else:
								cfile.write("((uint8_t) (p[{}] & (uint8_t) 0x{:02X}))".format(i, mask))
				cfile.write("));\n")
			cfile.write("}\n")
			cfile.write("\n")
	except:
		# On error, remove output files.
		os.remove(cfilename)
		os.remove(hfilename)
		raise
