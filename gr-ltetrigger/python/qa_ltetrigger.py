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
import pmt
import ltetrigger_swig as ltetrigger

class qa_ltetrigger(gr_unittest.TestCase):
    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def test_001_t(self):
        srcdir = os.getenv('srcdir') # set by gr testing framework
        data_fname = os.path.join(srcdir, 'lte_test_frames')

        # load 10 lte frames from data:
        # frames were created with the following parameters:
        # fc - 2400 MHz
        # nframes - 10
        # cell_id - 369
        # nof_prb 6 (1.4 MHz)

        # $ ./pdsch_enodeb -o lte_test_frames -f 2400 MHz -n 10 -c 369 -p 6
        # linux; GNU C++ version 5.2.1 20151010; Boost_105800; UHD_003.010.git-119-g42a3eeb6
        #
        # Using Volk machine: avx2_64_mmx
        #  - Resource Allocation Type:        Type 0
        #    + Resource Block Group Size:        1
        #    + RBG Bitmap:            0x3f
        #  - Modulation and coding scheme index:    1
        #  - HARQ process:            0
        #  - New data indicator:            No
        #  - Redundancy version:            0
        #  - TPC command for PUCCH:        --
        #  - PRB Bitmap Assignment 0st slot:
        # 0, 1, 2, 3, 4, 5,
        #  - PRB Bitmap Assignment 1st slot:
        # 0, 1, 2, 3, 4, 5,
        #  - Number of PRBs:            6
        #  - Modulation type:            QPSK
        #  - Transport block size:        208
        # Type new MCS index and press Enter: Done
        enodeb_data = np.fromfile(data_fname, dtype=np.complexfloating)
        vsrc = blocks.vector_source_c(enodeb_data, repeat=True)
        head = blocks.head(gr.sizeof_gr_complex, 100000)
        ltetrig = ltetrigger.ltetrigger()
        self.tb.connect(vsrc, head, ltetrig)

        # connect message passing interface
        msgdebug = blocks.message_debug()
        self.tb.msg_connect(ltetrig, 'trigger', msgdebug, 'store')

        # run enobeb_data through ltetrigger
        self.tb.run()

        nmsgs = msgdebug.num_messages()
        self.assertTrue(nmsgs, msg="No message triggered")

        for i in range(nmsgs):
            msg = pmt.to_python(msgdebug.get_message(i))

            # test ltetrigger conditions
            self.assertEqual(msg['link_type'], 'downlink', msg="wrong link_type")
            self.assertEqual(msg['cell_id'], 369, msg="wrong cell_id")


if __name__ == '__main__':
    import os
    print("Blocked waiting for GDB attach (pid = {})".format(os.getpid()))
    raw_input("Press Enter to continue...")

    gr_unittest.run(qa_ltetrigger, "qa_ltetrigger.xml")
