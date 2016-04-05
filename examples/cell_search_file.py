#!/usr/bin/env python2

from __future__ import print_function

import logging
import os
import sys
import time

from gnuradio import gr, blocks, eng_notation
from gnuradio import filter as gr_filter

import pmt

from ltetrigger import ltetrigger_c, ltetrigger_events


DATA_SIZE = gr.sizeof_gr_complex
REQUIRED_SAMPLE_RATE = 1.92e6


"""
Given a file containing a recorded LTE downlink, decode MIB and print to stdout.

Example usage:

$ ./cell_search_file.py -s 1.92M -f 2145M --repeat -c 19.2M ../gr-ltetrigger/python/lte_test_frames
Using Volk machine: avx2_64_mmx
{'nports': 1L, 'linktype': 'downlink', 'nprb': 6L, 'cell_id': 369L}
"""

class cell_search_file(gr.top_block):
    def __init__(self, args):
        gr.top_block.__init__(self)

        self.args = args

        self.logger = logging.getLogger('cell_search_file.top_block')

        fsource = blocks.file_source(DATA_SIZE,
                                     args.filename,
                                     repeat=args.repeat)
        if args.throttle:
            throttle = blocks.throttle(DATA_SIZE, args.throttle)

        if args.cut_off:
            cut_off = blocks.head(DATA_SIZE, args.cut_off)

        ltetrigger = ltetrigger_c()

        # TODO: set center_freq in ltetrigger

        if args.sample_rate % REQUIRED_SAMPLE_RATE:
            # Resampling will be costly, warn
            wrn  = "Sample rate {:.2f} MHz is not a multiple of 1.92 MHz. "
            wrn += "Performance will be poor."
            self.logger.warn(wrn.format(args.sample_rate))

        resamp_ratio = float(args.sample_rate) / float(REQUIRED_SAMPLE_RATE)
        resampler = gr_filter.fractional_resampler_cc(0, resamp_ratio)

        self.msg_store = blocks.message_debug()

        # Connect flowgraph
        lastblock = fsource

        if args.throttle:
            self.connect(lastblock, throttle)
            lastblock = throttle

        if args.cut_off:
            self.connect(lastblock, cut_off)
            lastblock = cut_off

        if resamp_ratio != 1:
            self.connect(lastblock, resampler)
            lastblock = resampler

        self.connect(lastblock, ltetrigger)

        self.msg_connect(ltetrigger, 'trigger',
                         self.msg_store, 'store')


def main(args):
    logger = logging.getLogger('cell_search_file.main')

    tb = cell_search_file(args)

    tb.start()

    while not tb.msg_store.num_messages():
        time.sleep(0.1)

    tb.stop()
    tb.wait()

    for i in range(tb.msg_store.num_messages()):
        print(pmt.to_python(tb.msg_store.get_message(i)))


if __name__ == '__main__':
    import argparse

    def eng_float(value):
        """Covert an argument string in engineering notation to float"""
        try:
            return eng_notation.str_to_num(value)
        except:
            msg = "invalid engineering notation value: {0!r}".format(value)
            raise argparse.ArgumentTypeError(msg)

    def eng_int(value):
        """Covert an argument string in engineering notation to int"""
        try:
            num = eng_notation.str_to_num(value)
            return int(num)
        except:
            msg = "invalid engineering notation value: {0!r}".format(value)
            raise argparse.ArgumentTypeError(msg)

    def filetype(fname):
        """Return fname if file exists, else raise ArgumentTypeError"""
        if os.path.isfile(fname):
            return fname
        else:
            errmsg = "file {} does not exist".format(fname)
            raise argparse.ArgumentTypeError(errmsg)


    parser = argparse.ArgumentParser()
    # Required
    parser.add_argument("filename", type=filetype)
    parser.add_argument("-s", "--sample-rate", type=eng_float, required=True,
                        metavar="Hz", help="input data's sample rate [Required]")
    parser.add_argument("-f", "--frequency", type=eng_float, required=True,
                        metavar="Hz", help="input data's center frequency [Required]")

    # Optional
    parser.add_argument("--repeat", action="store_true",
                        help="loop file until cell found or cut-off reached [default=%(default)s]")
    parser.add_argument("-c", "--cut-off", type=eng_int, metavar="N",
                        help="stop looping after N samples [default=%(default)s]")
    parser.add_argument("-t", "--throttle", type=eng_float, metavar="Hz",
                        help="throlle file source to lower CPU load [default=%(default)s]")
    args = parser.parse_args()
    main(args)
