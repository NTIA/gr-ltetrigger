#!/bin/sh
export VOLK_GENERIC=1
export GR_DONT_LOAD_PREFS=1
export srcdir=/home/dja/dev/ltetrigger/gr-ltetrigger/lib
export GR_CONF_CONTROLPORT_ON=False
export PATH=/home/dja/dev/ltetrigger/gr-ltetrigger/build/lib:$PATH
export LD_LIBRARY_PATH=/home/dja/dev/ltetrigger/gr-ltetrigger/build/lib:$LD_LIBRARY_PATH
export PYTHONPATH=$PYTHONPATH
test-ltetrigger 
