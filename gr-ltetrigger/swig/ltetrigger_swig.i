/* -*- c++ -*- */

#define LTETRIGGER_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "ltetrigger_swig_doc.i"

%{
#include "ltetrigger/pss.h"
#include "ltetrigger/sss.h"
#include "ltetrigger/mib.h"
#include "ltetrigger/cellstore.h"
%}

%include "ltetrigger/pss.h"
GR_SWIG_BLOCK_MAGIC2(ltetrigger, pss);

%include "ltetrigger/sss.h"
GR_SWIG_BLOCK_MAGIC2(ltetrigger, sss);

%include "ltetrigger/mib.h"
GR_SWIG_BLOCK_MAGIC2(ltetrigger, mib);

%include "ltetrigger/cellstore.h"
GR_SWIG_BLOCK_MAGIC2(ltetrigger, cellstore);
