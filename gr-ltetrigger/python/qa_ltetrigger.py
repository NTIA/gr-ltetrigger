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
        # - PRBs: 25 (20 MHz)
        # - Fc: 2400 MHz
        # - Modulation type: QPSK
        # - Transport block size: 904
        # - Cell ID: 369
        enodeb_data = np.fromfile(data_fname, dtype=np.complexfloating)
        vsrc = blocks.vector_source_c(enodeb_data, repeat=False)
        ltetrig = ltetrigger.ltetrigger()
        self.tb.connect(vsrc, ltetrig)

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
    gr_unittest.run(qa_ltetrigger, "qa_ltetrigger.xml")
