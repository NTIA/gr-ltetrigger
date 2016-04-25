#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2016 <+YOU OR YOUR COMPANY+>.
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

import os

import numpy as np

from gnuradio import gr, gr_unittest
from gnuradio import blocks
from gnuradio import filter as gr_filter
import pmt
import ltetrigger_swig as ltetrigger

class qa_ltetrigger_c(gr_unittest.TestCase):
    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def test_6PRB_t(self):
        srcdir = os.getenv('srcdir') # set by gr testing framework
        data_fname = os.path.join(srcdir, 'lte_frame_6PRB_cellid_123_2145MHz')

        # load an lte frame:

        # $ ./pdsch_enodeb -o lte_frame_6PRB_cellid_123_2145MHz -f 2145000000 -n 1 -c 123 -p 6
        # linux; GNU C++ version 5.2.1 20151010; Boost_105800; UHD_003.010.git-119-g42a3eeb6
        #
        # Using Volk machine: avx2_64_mmx
        #  - Resource Allocation Type:		Type 0
        #    + Resource Block Group Size:		1
        #    + RBG Bitmap:			0x3f
        #  - Modulation and coding scheme index:	1
        #  - HARQ process:			0
        #  - New data indicator:			No
        #  - Redundancy version:			0
        #  - TPC command for PUCCH:		--
        #  - PRB Bitmap Assignment 0st slot:
        # 0, 1, 2, 3, 4, 5,
        #  - PRB Bitmap Assignment 1st slot:
        # 0, 1, 2, 3, 4, 5,
        #  - Number of PRBs:			6
        #  - Modulation type:			QPSK
        #  - Transport block size:		208
        # Type new MCS index and press Enter: Done

        fsrc = blocks.file_source(gr.sizeof_gr_complex, data_fname, repeat=True)
        head = blocks.head(gr.sizeof_gr_complex, 5000000)
        ltetrig_c = ltetrigger.ltetrigger_c()
        self.tb.connect(fsrc, head, ltetrig_c)

        # connect message passing interface
        msgdebug = blocks.message_debug()
        self.tb.msg_connect(ltetrig_c, 'trigger', msgdebug, 'store')
        self.tb.msg_connect(ltetrig_c, 'trigger', msgdebug, 'print')

        # run enobeb_data through ltetrigger
        self.tb.run()

        nmsgs = msgdebug.num_messages()
        self.assertTrue(nmsgs, msg="No message triggered")

        for i in range(nmsgs):
            msg = pmt.to_python(msgdebug.get_message(i))

            # test ltetrigger conditions
            self.assertEqual(msg['linktype'], 'downlink', msg="wrong linktype")
            self.assertEqual(msg['cell_id'], 123, msg="wrong cell_id")

    def test_25PRB_t(self):
        srcdir = os.getenv('srcdir') # set by gr testing framework
        data_fname = os.path.join(srcdir, 'lte_frame_25PRB_cellid_124_2145MHz')

        # load an lte frame:

        # $ ./pdsch_enodeb -o lte_frame_25PRB_cellid_124_2145MHz -f 2145000000 -n 1 -c 124 -p 25
        # linux; GNU C++ version 5.2.1 20151010; Boost_105800; UHD_003.010.git-119-g42a3eeb6
        #
        # Using Volk machine: avx2_64_mmx
        #  - Resource Allocation Type:		Type 0
        #    + Resource Block Group Size:		2
        #    + RBG Bitmap:			0x1fff
        #  - Modulation and coding scheme index:	1
        #  - HARQ process:			0
        #  - New data indicator:			No
        #  - Redundancy version:			0
        #  - TPC command for PUCCH:		--
        #  - PRB Bitmap Assignment 0st slot:
        # 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
        #  - PRB Bitmap Assignment 1st slot:
        # 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
        #  - Number of PRBs:			25
        #  - Modulation type:			QPSK
        #  - Transport block size:		904
        # Type new MCS index and press Enter: Done

        fsrc = blocks.file_source(gr.sizeof_gr_complex, data_fname, repeat=True)
        head = blocks.head(gr.sizeof_gr_complex, 5000000)
        output_rate = 1.92e6
        input_rate = 7.68e6
        resamp_ratio = input_rate / output_rate # 4.0
        resamp = gr_filter.fractional_resampler_cc(0, resamp_ratio)
        ltetrig_c = ltetrigger.ltetrigger_c()
        self.tb.connect(fsrc, head, resamp, ltetrig_c)

        # connect message passing interface
        msgdebug = blocks.message_debug()
        self.tb.msg_connect(ltetrig_c, 'trigger', msgdebug, 'store')
        self.tb.msg_connect(ltetrig_c, 'trigger', msgdebug, 'print')

        # run enobeb_data through ltetrigger
        self.tb.run()

        nmsgs = msgdebug.num_messages()
        self.assertTrue(nmsgs, msg="No message triggered")

        for i in range(nmsgs):
            msg = pmt.to_python(msgdebug.get_message(i))

            # test ltetrigger conditions
            self.assertEqual(msg['linktype'], 'downlink', msg="wrong linktype")
            self.assertEqual(msg['cell_id'], 124, msg="wrong cell_id")

    def test_50PRB_t(self):
        srcdir = os.getenv('srcdir') # set by gr testing framework
        data_fname = os.path.join(srcdir, 'lte_frame_50PRB_cellid_125_2145MHz')

        # load an lte frame:

        # $ ./pdsch_enodeb -o lte_frame_50PRB_cellid_125_2145MHz -f 2145000000 -n 1 -c 125 -p 50
        # linux; GNU C++ version 5.2.1 20151010; Boost_105800; UHD_003.010.git-119-g42a3eeb6
        #
        # Using Volk machine: avx2_64_mmx
        #  - Resource Allocation Type:		Type 0
        #    + Resource Block Group Size:		3
        #    + RBG Bitmap:			0x1ffff
        #  - Modulation and coding scheme index:	1
        #  - HARQ process:			0
        #  - New data indicator:			No
        #  - Redundancy version:			0
        #  - TPC command for PUCCH:		--
        #  - PRB Bitmap Assignment 0st slot:
        # 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
        #  - PRB Bitmap Assignment 1st slot:
        # 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
        #  - Number of PRBs:			50
        #  - Modulation type:			QPSK
        #  - Transport block size:		1800
        # Type new MCS index and press Enter: Done

        fsrc = blocks.file_source(gr.sizeof_gr_complex, data_fname, repeat=True)
        head = blocks.head(gr.sizeof_gr_complex, 5000000)
        ltetrig_c = ltetrigger.ltetrigger_c()
        self.tb.connect(fsrc, head, ltetrig_c)

        # connect message passing interface
        msgdebug = blocks.message_debug()
        self.tb.msg_connect(ltetrig_c, 'trigger', msgdebug, 'store')
        self.tb.msg_connect(ltetrig_c, 'trigger', msgdebug, 'print')

        # run enobeb_data through ltetrigger
        self.tb.run()

        nmsgs = msgdebug.num_messages()
        self.assertTrue(nmsgs, msg="No message triggered")

        for i in range(nmsgs):
            msg = pmt.to_python(msgdebug.get_message(i))

            # test ltetrigger conditions
            self.assertEqual(msg['linktype'], 'downlink', msg="wrong linktype")
            self.assertEqual(msg['cell_id'], 125, msg="wrong cell_id")

    def test_100PRB_t(self):
        srcdir = os.getenv('srcdir') # set by gr testing framework
        data_fname = os.path.join(srcdir, 'lte_frame_100PRB_cellid_369_2145MHz')

        # load an lte frame:

        # $ ./pdsch_enodeb -o lte_frame_100PRB_cellid_369_2145MHz -f 2145000000 -n 1 -c 369 -p 100
        # linux; GNU C++ version 5.2.1 20151010; Boost_105800; UHD_003.010.git-119-g42a3eeb6
        #
        # Using Volk machine: avx2_64_mmx
        #  - Resource Allocation Type:		Type 0
        #    + Resource Block Group Size:		4
        #    + RBG Bitmap:			0x1ffffff
        #  - Modulation and coding scheme index:	1
        #  - HARQ process:			0
        #  - New data indicator:			No
        #  - Redundancy version:			0
        #  - TPC command for PUCCH:		--
        #  - PRB Bitmap Assignment 0st slot:
        # 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
        #  - PRB Bitmap Assignment 1st slot:
        # 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
        #  - Number of PRBs:			100
        #  - Modulation type:			QPSK
        #  - Transport block size:		3624
        # Type new MCS index and press Enter: Done

        fsrc = blocks.file_source(gr.sizeof_gr_complex, data_fname, repeat=True)
        head = blocks.head(gr.sizeof_gr_complex, 5000000)
        ltetrig_c = ltetrigger.ltetrigger_c()
        self.tb.connect(fsrc, head, ltetrig_c)

        # connect message passing interface
        msgdebug = blocks.message_debug()
        self.tb.msg_connect(ltetrig_c, 'trigger', msgdebug, 'store')
        self.tb.msg_connect(ltetrig_c, 'trigger', msgdebug, 'print')

        # run enobeb_data through ltetrigger
        self.tb.run()

        nmsgs = msgdebug.num_messages()
        self.assertTrue(nmsgs, msg="No message triggered")

        for i in range(nmsgs):
            msg = pmt.to_python(msgdebug.get_message(i))

            # test ltetrigger conditions
            self.assertEqual(msg['linktype'], 'downlink', msg="wrong linktype")
            self.assertEqual(msg['cell_id'], 369, msg="wrong cell_id")


if __name__ == '__main__':
    #import os
    #print("Blocked waiting for GDB attach (pid = {})".format(os.getpid()))
    #raw_input("Press Enter to continue...")

    gr_unittest.run(qa_ltetrigger_c, "qa_ltetrigger_c.xml")
