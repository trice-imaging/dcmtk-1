# Message from original patch:
	This repo contains the latest snapshot of dcmtk (10/2015).

	dcmj2pnm has memory problems if the size of the decompressed pixel data exeeds the sizeof(size_t) (ie., 4 bytes). 
	In this case the unsigned long overflows and the size to be allocated is incorrect.

	This will either lead to a crash or corrupt output files.

	dcm2pnm.cc is included in several utilities.  To minimize risk, a copy of this file is made so that only dcmj2pnm is built with these changes.
	The changes include looping through the frames and writing out each outputfile (instead of swallowing the whole decompressed file into memory).
	This approach uses very little memory.

	original dcm2pnm: dcmimage/apps/dcm2pnm
	modified dcm2pnm: dcmjpeg/apps/dcm2pnm
	=======
#

This DICOM ToolKit (DCMTK) package consists of source code, documentation and installation instructions for a set of software libraries and applications implementing part of the DICOM/MEDICOM Standard.

DCMTK contains the following sub-packages, each in its own sub-directory:

- **config**   - configuration utilities for DCMTK
- **dcmdata**  - a data encoding/decoding library and utility apps
- **dcmect**   - a library for working with Enhanced CT objects
- **dcmfg**    - a library for working with functional groups
- **dcmimage** - adds support for color images to dcmimgle
- **dcmimgle** - an image processing library and utility apps
- **dcmiod**   - a library for working with information objects and modules
- **dcmjpeg**  - a compression/decompression library and utility apps
- **dcmjpls**  - a compression/decompression library and utility apps
- **dcmnet**   - a networking library and utility apps
- **dcmpmap**  - a library for working with parametric map objects
- **dcmpstat** - a presentation state library and utility apps
- **dcmqrdb**  - an image database server
- **dcmrt**    - a radiation therapy library and utility apps
- **dcmseg**   - a library for working with segmentation objects
- **dcmsign**  - a digital signature library and utility apps
- **dcmsr**    - a structured reporting library and utility apps
- **dcmtls**   - security extensions for the network library
- **dcmtract** - a library for working with tractography results
- **dcmwlm**   - a modality worklist database server
- **oflog**    - a logging library based on log4cplus
- **ofstd**    - a library of general purpose classes

Each sub-directory (except _config_) contains further sub-directories for application source code (_apps_), library source code (_libsrc_), library include files (_include_), configuration data (_etc_), documentation (_docs_), sample and support data (_data_) as well as test programs (_tests_).

To build and install the DCMTK package see the [INSTALL](INSTALL) file.  For copyright information see the [COPYRIGHT](COPYRIGHT) file.  For information about the history of this software see the [HISTORY](HISTORY) file.  For answers to frequently asked questions please consult the [FAQ](https://forum.dcmtk.org/faq/).

In addition to the API documentation, which is also available [online](https://support.dcmtk.org/docs/), there is a [Wiki](https://support.dcmtk.org/wiki/) system where further information (e.g. HOWTOs) can be found.

If you find bugs or other problems with this software, we would appreciate hearing about them.  Please send electronic mail to: bugs/at/dcmtk/dot/org

Please try to describe the problem in detail and if possible give a suggested fix.  For general questions on how to compile, install or use the toolkit we recommend the [public discussion forum](https://forum.dcmtk.org/).
