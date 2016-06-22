#!/usr/bin/env python2


"""
Given a file containing a recorded LTE downlink, decode MIB and print to stdout.
Example usage:
$ ./cell_search_file.py -s 1.92M -f 2145M --repeat -c 19.2M ../gr-ltetrigger/python/lte_test_frames
Using Volk machine: avx2_64_mmx
{'nports': 1L, 'linktype': 'downlink', 'nprb': 6L, 'cell_id': 369L}
"""


from __future__ import print_function

import logging
import json
import os
import sys
import time

from gnuradio import gr, blocks, eng_notation
from gnuradio import filter as gr_filter

import pmt

from ltetrigger import downlink_trigger_c, cellstore


DATA_SIZE = gr.sizeof_gr_complex
REQUIRED_SAMPLE_RATE = 1.92e6


class cell_search_file(gr.top_block):
    def __init__(self, args):
        gr.top_block.__init__(self)

        self.args = args

        self.logger = logging.getLogger('cell_search_file')

        fsource = blocks.file_source(DATA_SIZE,
                                     args.filename,
                                     repeat=args.repeat)
        if args.throttle:
            throttle = blocks.throttle(DATA_SIZE, args.throttle)

        if args.cut_off > -1:
            cut_off = blocks.head(DATA_SIZE, args.cut_off)

        if args.sample_rate % REQUIRED_SAMPLE_RATE:
            err  = "Sample rate {:.2f} MHz is not a multiple of 1.92 MHz. "
            err += "Arbitrary resampling not supported at this time."
            self.logger.error(err.format(args.sample_rate))
            sys.exit(-1)

        resamp_ratio = int(args.sample_rate / REQUIRED_SAMPLE_RATE)
        self.resampler = gr_filter.rational_resampler_ccc(1, resamp_ratio)

        self.trigger = downlink_trigger_c(psr_threshold=args.threshold,
                                          exit_on_success=True)

        self.cellstore = cellstore()

        # TODO: pass center_freq in to trigger

        # Connect flowgraph
        lastblock = fsource

        if resamp_ratio != 1:
            self.connect(lastblock, self.resampler)
            lastblock = self.resampler

        if args.throttle:
            self.connect(lastblock, throttle)
            lastblock = throttle

        if args.cut_off > -1:
            self.connect(lastblock, cut_off)
            lastblock = cut_off

        self.connect((lastblock, 0), self.trigger)

        track_port_id = "track"
        self.msg_connect(self.trigger, track_port_id,
                         self.cellstore, track_port_id)

        drop_port_id = "drop"
        self.msg_connect(self.trigger, drop_port_id,
                         self.cellstore, drop_port_id)


def main(args):
    logging.basicConfig()
    logger = logging.getLogger('cell_search_file.main')

    tb = cell_search_file(args)

    print("Starting cell search... ", end='')
    sys.stdout.flush()

    tb.start()

    if args.cut_off == -1 and args.time_out > -1:
        t_start = t_now = time.time()
        while t_now - t_start < args.time_out:
            time.sleep(0.1)
            t_now = time.time()

        tb.stop()

    tb.wait()

    print("done.")

    results = []

    if tb.cellstore.tracking_any():
        if tb.cellstore.is_tracking0():
            cell0 = pmt.to_python(tb.cellstore.cell0())
            cell0["status"] = "FOUND"
            cell0_json = json.dumps(cell0,
                                    default=lambda o: o.__dict__,
                                    indent=4)
            results.append(cell0_json)

        if tb.cellstore.is_tracking1():
            cell1 = pmt.to_python(tb.cellstore.cell1())
            cell1["status"] = "FOUND"
            cell1_json = json.dumps(cell1,
                                    default=lambda o: o.__dict__,
                                    indent=4)
            results.append(cell1_json)

        if tb.cellstore.is_tracking2():
            cell2 = pmt.to_python(tb.cellstore.cell2())
            cell2["status"] = "FOUND"
            cell2_json = json.dumps(cell2,
                                    default=lambda o: o.__dict__,
                                    indent=4)
            results.append(cell2_json)
    else:
        result = {"status": "NOT_FOUND"}
        result_json = json.dumps(result)
        results.append(result_json)

    for cell in results:
        print(cell)

    if args.fifoname is not None:
        if not os.path.exists(args.fifoname):
            os.mkfifo(args.fifoname)
            pipeout = os.open(args.fifoname, os.O_WRONLY)
            for cell in results:
                os.write(pipeout, str(len(cell)) + "\n" + cell)
            os.close(pipeout)


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
                        metavar="Hz", help="input data's sample rate " +
                        "[Required]")
    # Optional
    parser.add_argument("-f", "--frequency", type=eng_float,
                        metavar="Hz", help="input data's center frequency")
    parser.add_argument("--repeat", action='store_true',
                        help="loop file until cell found or cut-off reached " +
                        "[default=%(default)s]")
    parser.add_argument("-c", "--cut-off", type=eng_int, metavar="N", default=-1,
                        help="stop looping after N samples " +
                        "[default=%(default)s]")
    parser.add_argument("--throttle", type=eng_float, metavar="Hz",
                        help="throttle file source to lower CPU load " +
                        "[default=%(default)s]")
    parser.add_argument("--time-out", type=eng_float, metavar="sec", default=-1,
                        help="max time in seconds to perform search " +
                        "[default=%(default)s]")
    parser.add_argument("--threshold", type=eng_float, default=4,
                        help="set peak to side-lobe ratio threshold " +
                        "[default=%(default)s]")
    parser.add_argument("--gui", action='store_true', help=argparse.SUPPRESS)
    parser.add_argument("--debug", action='store_true', help=argparse.SUPPRESS)
    parser.add_argument("--fifoname", default=None, required=False, help = "FIFO name to which to write output")
    args = parser.parse_args()

    if args.debug:
        print("Blocked waiting for GDB attach (pid = {})".format(os.getpid()))
        raw_input("Press enter to continue...")

    main(args)
