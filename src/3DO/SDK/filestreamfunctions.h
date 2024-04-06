#ifndef FILESTREAMFUNCTIONS_H
#define FILESTREAMFUNCTIONS_H

#include "types.h"
#include "filestream.h"

int32 ReadDiskStream(Stream *theStream, char *buffer, int32 nBytes);

#define OpenDiskStream(filepath,l) fopen(filepath, "rb")
#define CloseDiskStream(stream) fclose(stream)
#define SeekDiskStream(stream,offset,e) fseek(stream,offset,e)
#define	ReadDiskStream(stream,buffer,size) fread(buffer,size,1,stream)

#endif
