/* -*- C -*-
 * File: libraw_datastream.h
 * Copyright 2008-2010 LibRaw LLC (info@libraw.org)
 * Created: Sun Jan 18 13:07:35 2009
 *
 * LibRaw Data stream interface

LibRaw is free software; you can redistribute it and/or modify
it under the terms of the one of three licenses as you choose:

1. GNU LESSER GENERAL PUBLIC LICENSE version 2.1
   (See file LICENSE.LGPL provided in LibRaw distribution archive for details).

2. COMMON DEVELOPMENT AND DISTRIBUTION LICENSE (CDDL) Version 1.0
   (See file LICENSE.CDDL provided in LibRaw distribution archive for details).

3. LibRaw Software License 27032010
   (See file LICENSE.LibRaw.pdf provided in LibRaw distribution archive for details).

 */

#ifndef __LIBRAW_DATASTREAM_H
#define __LIBRAW_DATASTREAM_H

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#ifndef __cplusplus

struct LibRaw_abstract_datastream;

#else /* __cplusplus */

#include "libraw_const.h"
#include "libraw_types.h"
#include <fstream>
#include <memory>

class LibRaw_buffer_datastream;

class LibRaw_abstract_datastream
{
  public:
    LibRaw_abstract_datastream(){substream=0;};
    virtual             ~LibRaw_abstract_datastream(void){if(substream) delete substream;}
    virtual int         valid(){return 0;}
    virtual int         read(void *,size_t, size_t ){ return -1;}
    virtual int         seek(INT64 , int ){return -1;}
    virtual INT64       tell(){return -1;}
    virtual int         get_char(){return -1;}
    virtual char*       gets(char *, int){ return NULL;}
    virtual int         scanf_one(const char *, void *){return -1;}
    virtual int         eof(){return -1;}

    virtual const char* fname(){ return NULL;};
    virtual int         subfile_open(const char*){ return EINVAL;}
    virtual void        subfile_close(){}
    virtual int		tempbuffer_open(void*, size_t);
    virtual void	tempbuffer_close()
    {
        if(substream) delete substream;
        substream = NULL;
    }

  protected:
    LibRaw_abstract_datastream *substream;
};


class LibRaw_file_datastream : public LibRaw_abstract_datastream
{
  public:
    LibRaw_file_datastream(const char *fname) 
    { 
      if(fname) {
        filename = fname;
        f.reset(new std::filebuf());
        f->open(fname, std::ios_base::in | std::ios_base::binary);
        if (!f->is_open()) f.reset();
      } else  {
        filename=0;
      }
    }

    virtual int valid() { return f.get()?1:0;}

#define CHK() do {if(!f.get()) throw LIBRAW_EXCEPTION_IO_EOF;}while(0)
    virtual int read(void * ptr,size_t size, size_t nmemb) 
    { 
        CHK(); 
        if (substream) return substream->read(ptr, size, nmemb);

        return f->sgetn(static_cast<char*>(ptr), nmemb * size) / size;
    }
    virtual int eof() 
    { 
        CHK(); 
        if (substream) return substream->eof();
        return f->sgetc() == EOF;
    }
    virtual int seek(INT64 o, int whence) 
    { 
        CHK(); 
        if (substream) return substream->seek(o, whence);

        std::ios_base::seekdir dir;
        switch (whence) {
        case SEEK_SET: dir = std::ios_base::beg; break;
        case SEEK_CUR: dir = std::ios_base::cur; break;
        case SEEK_END: dir = std::ios_base::end; break;
        default: dir = std::ios_base::beg;
        }
        return f->pubseekoff(o, dir);
    }
    virtual INT64 tell() 
    { 
        CHK(); 
        if (substream) return substream->tell();
        return f->pubseekoff(0, std::ios_base::cur);
    }
    virtual int get_char() 
    { 
        CHK(); 
        if (substream) return substream->get_char();
        return f->sbumpc();
    }
    virtual char* gets(char *str, int sz) 
    { 
        CHK(); 
        if (substream) return substream->gets(str, sz);

        std::istream is(f.get());
        is.getline(str, sz);
        if (is.fail()) return 0;
        return str;
    }
    virtual int scanf_one(const char *fmt, void*val) 
    { 
        CHK(); 
        if (substream) return substream->scanf_one(fmt, val);

        std::istream is(f.get());

        /* HUGE ASSUMPTION: *fmt is either "%d" or "%f" */
        if (strcmp(fmt, "%d") == 0) {
          int d;
          is >> d;
          if (is.fail()) return EOF;
          *(static_cast<int*>(val)) = d;
        } else {
          float f;
          is >> f;
          if (is.fail()) return EOF;
          *(static_cast<float*>(val)) = f;
        }

        return 1;
    }

    virtual const char *fname() { return filename; }

    virtual int subfile_open(const char *fn)
    {
        if (saved_f.get()) return EBUSY;

        saved_f = f;

        f.reset(new std::filebuf());
        f->open(fn, std::ios_base::in | std::ios_base::binary);
        if (!f->is_open()) {
          f = saved_f;
          return ENOENT;
        }
        else
          return 0;
    }
    virtual void subfile_close()
    {
        if (!saved_f.get()) return;
        f = saved_f;
    }

  private:
    std::auto_ptr<std::filebuf> f; /* will close() automatically through dtor */
    std::auto_ptr<std::filebuf> saved_f; /* when *f is a subfile, *saved_f is the master file */
    const char *filename;
};
#undef CHK

class LibRaw_buffer_datastream : public LibRaw_abstract_datastream
{
  public:
    LibRaw_buffer_datastream(void *buffer, size_t bsize)
        {
            buf = (unsigned char*)buffer; streampos = 0; streamsize = bsize;
        }
    virtual ~LibRaw_buffer_datastream(){}
    virtual int valid() { return buf?1:0;}
    virtual int read(void * ptr,size_t sz, size_t nmemb) 
    { 
        if(substream) return substream->read(ptr,sz,nmemb);
        size_t to_read = sz*nmemb;
        if(to_read > streamsize - streampos)
            to_read = streamsize-streampos;
        if(to_read<1) 
            return 0;
        memmove(ptr,buf+streampos,to_read);
        streampos+=to_read;
        return int((to_read+sz-1)/sz);
    }

    virtual int eof() 
    { 
        if(substream) return substream->eof();
        return streampos >= streamsize;
    }

    virtual int seek(INT64 o, int whence) 
    { 
        if(substream) return substream->seek(o,whence);
        switch(whence)
            {
            case SEEK_SET:
                if(o<0)
                    streampos = 0;
                else if (size_t(o) > streamsize)
                    streampos = streamsize;
                else
                    streampos = size_t(o);
                return 0;
            case SEEK_CUR:
                if(o<0)
                    {
                        if(size_t(-o) >= streampos)
                            streampos = 0;
                        else
                            streampos += (size_t)o;
                    }
                else if (o>0)
                    {
                        if(o+streampos> streamsize)
                            streampos = streamsize;
                        else
                            streampos += (size_t)o;
                    }
                return 0;
            case SEEK_END:
                if(o>0)
                    streampos = streamsize;
                else if ( size_t(-o) > streamsize)
                    streampos = 0;
                else
                    streampos = streamsize+(size_t)o;
                return 0;
            default:
                return 0;
            }
    }
    
    virtual INT64 tell() 
    { 
        if(substream) return substream->tell();
        return INT64(streampos);
    }

    virtual int get_char() 
    { 
        if(substream) return substream->get_char();
        if(streampos>=streamsize)
            return -1;
        return buf[streampos++];
    }
    virtual char* gets(char *s, int sz) 
    { 
        if (substream) return substream->gets(s,sz);
        unsigned char *psrc,*pdest,*str;
        str = (unsigned char *)s;
        psrc = buf+streampos;
        pdest = str;
        while ( (size_t(psrc - buf) < streamsize)
               &&
                ((pdest-str)<sz)
		)
	  {
                *pdest = *psrc;
                if(*psrc == '\n')
                    break;
                psrc++;
                pdest++;
            }
        if(size_t(psrc-buf) < streamsize)
            psrc++;
        if((pdest-str)<sz)
            *(++pdest)=0;
        streampos = psrc - buf;
        return s;
    }
    virtual int scanf_one(const char *fmt, void* val) 
    { 
        if(substream) return substream->scanf_one(fmt,val);
        int scanf_res;
        if(streampos>streamsize) return 0;
        scanf_res = sscanf((char*)(buf+streampos),fmt,val);
        if(scanf_res>0)
            {
                int xcnt=0;
                while(streampos<streamsize)
                    {
                        streampos++;
                        xcnt++;
                        if(buf[streampos] == 0
                           || buf[streampos]==' '
                           || buf[streampos]=='\t'
                           || buf[streampos]=='\n'
                           || xcnt>24)
                            break;
                    }
            }
        return scanf_res;
    }
  private:
    unsigned char *buf;
    size_t   streampos,streamsize;
};

inline int LibRaw_abstract_datastream::tempbuffer_open(void  *buf, size_t size)
{
    if(substream) return EBUSY;
    substream = new LibRaw_buffer_datastream(buf,size);
    return substream?0:EINVAL;
}


#endif

#endif

