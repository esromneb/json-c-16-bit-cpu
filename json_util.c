/*
 * $Id: json_util.c,v 1.4 2006/01/30 23:07:57 mclark Exp $
 *
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Michael Clark <michael@metaparadigm.com>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See COPYING for details.
 *
 */

#include "config.h"
#undef realloc

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#if HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <io.h>
#endif /* defined(WIN32) */

#if !HAVE_OPEN && defined(WIN32)
# define open _open
#endif


#include "bits.h"
#include "debug.h"
#include "printbuf.h"
#include "json_inttypes.h"
#include "json_object.h"
#include "json_tokener.h"
#include "json_util.h"

struct json_object* json_object_from_file(const char *filename)
{
  struct printbuf *pb;
  struct json_object *obj;
  char buf[JSON_FILE_BUF_SIZE];
  int fd, ret;

  if((fd = open(filename, O_RDONLY)) < 0) {
    MC_ERROR("json_object_from_file: error reading file %s: %s\n",
	     filename, strerror(errno));
    return (struct json_object*)error_ptr(-1);
  }
  if(!(pb = printbuf_new())) {
    close(fd);
    MC_ERROR("json_object_from_file: printbuf_new failed\n");
    return (struct json_object*)error_ptr(-1);
  }
  while((ret = read(fd, buf, JSON_FILE_BUF_SIZE)) > 0) {
    printbuf_memappend(pb, buf, ret);
  }
  close(fd);
  if(ret < 0) {
    MC_ABORT("json_object_from_file: error reading file %s: %s\n",
	     filename, strerror(errno));
    printbuf_free(pb);
    return (struct json_object*)error_ptr(-1);
  }
  obj = json_tokener_parse(pb->buf);
  printbuf_free(pb);
  return obj;
}

int json_object_to_file(char *filename, struct json_object *obj)
{
  const char *json_str;
  int fd, ret;
  unsigned int wpos, wsize;

  if(!obj) {
    MC_ERROR("json_object_to_file: object is null\n");
    return -1;
  }

  if((fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0644)) < 0) {
    MC_ERROR("json_object_to_file: error opening file %s: %s\n",
	     filename, strerror(errno));
    return -1;
  }

  if(!(json_str = json_object_to_json_string(obj))) {
    close(fd);
    return -1;
  }

  wsize = (unsigned int)(strlen(json_str) & UINT_MAX); /* CAW: probably unnecessary, but the most 64bit safe */
  wpos = 0;
  while(wpos < wsize) {
    if((ret = write(fd, json_str + wpos, wsize-wpos)) < 0) {
      close(fd);
      MC_ERROR("json_object_to_file: error writing file %s: %s\n",
	     filename, strerror(errno));
      return -1;
    }

	/* because of the above check for ret < 0, we can safely cast and add */
    wpos += (unsigned int)ret;
  }

  close(fd);
  return 0;
}

#if HAVE_REALLOC == 0
void* rpl_realloc(void* p, size_t n)
{
	if (n == 0)
		n = 1;
	if (p == 0)
		return malloc(n);
	return realloc(p, n);
}
#endif

#define NELEM(a)        (sizeof(a) / sizeof(a[0]))
static const char* json_type_name[] = {
  /* If you change this, be sure to update the enum json_type definition too */
  "null",
  "boolean",
  "double",
  "int",
  "object",
  "array",
  "string",
};

const char *json_type_to_name(enum json_type o_type)
{
	if (o_type < 0 || o_type >= NELEM(json_type_name))
	{
		MC_ERROR("json_type_to_name: type %d is out of range [0,%d]\n", o_type, NELEM(json_type_name));
		return NULL;
	}
	return json_type_name[o_type];
}

