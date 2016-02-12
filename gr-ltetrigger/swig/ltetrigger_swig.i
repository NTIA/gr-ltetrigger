/* -*- c++ -*- */

#define LTETRIGGER_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "ltetrigger_swig_doc.i"

%{
#include "ltetrigger/ltetrigger.h"
%}


%include "ltetrigger/ltetrigger.h"
GR_SWIG_BLOCK_MAGIC2(ltetrigger, ltetrigger);
