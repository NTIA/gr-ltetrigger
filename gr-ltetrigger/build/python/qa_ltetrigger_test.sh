#!/bin/sh
export VOLK_GENERIC=1
export GR_DONT_LOAD_PREFS=1
export srcdir=/home/dja/dev/ltetrigger/gr-ltetrigger/python
export GR_CONF_CONTROLPORT_ON=False
export PATH=/home/dja/dev/ltetrigger/gr-ltetrigger/build/python:$PATH
export LD_LIBRARY_PATH=/home/dja/dev/ltetrigger/gr-ltetrigger/build/lib:$LD_LIBRARY_PATH
export PYTHONPATH=/home/dja/dev/ltetrigger/gr-ltetrigger/build/swig:$PYTHONPATH
/usr/bin/python2 /home/dja/dev/ltetrigger/gr-ltetrigger/python/qa_ltetrigger.py 
