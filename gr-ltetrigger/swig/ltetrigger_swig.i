/* -*- c++ -*- */

#define LTETRIGGER_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "ltetrigger_swig_doc.i"

%{
#include "ltetrigger/cfo.h"
#include "ltetrigger/pss.h"
#include "ltetrigger/sss.h"
%}


%include "ltetrigger/cfo.h"
GR_SWIG_BLOCK_MAGIC2(ltetrigger, cfo);
%include "ltetrigger/pss.h"
GR_SWIG_BLOCK_MAGIC2(ltetrigger, pss);

%include "ltetrigger/sss.h"
GR_SWIG_BLOCK_MAGIC2(ltetrigger, sss);
