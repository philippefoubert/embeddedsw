###############################################################################
#
# Copyright (C) 2013 - 2017 Xilinx, Inc.  All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# Use of the Software is limited solely to applications:
# (a) running on a Xilinx device, or
# (b) that interact with a Xilinx device through a bus or interconnect.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
# OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Except as contained in this notice, the name of the Xilinx shall not be used
# in advertising or otherwise to promote the sale, use or other dealings in
# this Software without prior written authorization from Xilinx.
#
###############################################################################
#
# Modification History
#
# Ver   Who  Date     Changes
# ----- ---- -------- -----------------------------------------------
# 1.00a ba  06/01/15 Initial Release
# 1.2   vns 08/23/16 Added support for SHA2 by adding .a files
# 2.0   vns 11/28/16  Added support for PMU
##############################################################################

#---------------------------------------------
# secure_drc
#---------------------------------------------
proc secure_drc {libhandle} {
	# check processor type
	set proc_instance [hsi::get_sw_processor];
	set hw_processor [common::get_property HW_INSTANCE $proc_instance]
	set compiler [common::get_property CONFIG.compiler $proc_instance]
	set proc_type [common::get_property IP_NAME [hsi::get_cells -hier $hw_processor]];

	if { $proc_type != "psu_cortexa53" && $proc_type != "psu_cortexr5" && $proc_type != "psu_pmu" } {
				error "ERROR: XilSecure library is supported only for PMU, CortexA53 and CortexR5 processors.";
				return;
	}

	#Copying .a file based on compiler
	if {[string compare -nocase $compiler "arm-none-eabi-gcc"] == 0} {
		file delete -force ./src/xsecure_sha2_pmu.a
		file delete -force ./src/xsecure_sha2_r5.a
		file delete -force ./src/xsecure_sha2_a53_64b.a
		file rename -force ./src/xsecure_sha2_a53_32b.a ./src/libxilsecure.a
	} elseif {[string compare -nocase $compiler "aarch64-none-elf-gcc"] == 0} {
		file delete -force ./src/xsecure_sha2_pmu.a
		file delete -force ./src/xsecure_sha2_r5.a
		file delete -force ./src/xsecure_sha2_a53_32b.a
		file rename -force ./src/xsecure_sha2_a53_64b.a ./src/libxilsecure.a
	} elseif {[string compare -nocase $compiler "armr5-none-eabi-gcc"] == 0} {
		file delete -force ./src/xsecure_sha2_pmu.a
		file delete -force ./src/xsecure_sha2_a53_32b.a
		file delete -force ./src/xsecure_sha2_a53_64b.a
		file rename -force ./src/xsecure_sha2_r5.a ./src/libxilsecure.a
	} elseif {[string compare -nocase $compiler "mb-gcc"] == 0} {
		file delete -force ./src/xsecure_sha2_r5.a
		file delete -force ./src/xsecure_sha2_a53_32b.a
		file delete -force ./src/xsecure_sha2_a53_64b.a
		file rename -force ./src/xsecure_sha2_pmu.a ./src/libxilsecure.a
	}

}

proc generate {libhandle} {

}

#-------
# post_generate: called after generate called on all libraries
#-------
proc post_generate {libhandle} {

	xgen_opts_file $libhandle
}

#-------
# execs_generate: called after BSP's, libraries and drivers have been compiled
#	This procedure builds the libxilsecure.a library
#-------
proc execs_generate {libhandle} {

}

proc xgen_opts_file {libhandle} {

	# Copy the include files to the include directory
	set srcdir src
	set dstdir [file join .. .. include]

	# Create dstdir if it does not exist
	if { ! [file exists $dstdir] } {
		file mkdir $dstdir
	}

	# Get list of files in the srcdir
	set sources [glob -join $srcdir *.h]

	# Copy each of the files in the list to dstdir
	foreach source $sources {
		file copy -force $source $dstdir
	}
}
