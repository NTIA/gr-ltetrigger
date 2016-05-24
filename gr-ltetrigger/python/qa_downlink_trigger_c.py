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
from pprint import pprint

from gnuradio import gr, gr_unittest
from gnuradio import blocks
import gnuradio.filter as gr_filter

import pmt

from downlink_trigger_c import downlink_trigger_c


REQUIRED_SAMPLE_RATE = 1.92e6  # 1.92 MHz


class qa_downlink_trigger_c(gr_unittest.TestCase):
    def setUp(self):
        self.psr_threshold = 4
        self.exit_on_success = True
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def _check_cell_id(self, cell, expected_id):
        self.assertEqual(cell['cell_id'], expected_id, msg="wrong cell_id")

    def _check_cp_len(self, cell):
        self.assertEqual(cell['cp_len'], "Normal", msg="wrong cp_len")

    def _check_nof_phich_resources(self, cell):
        err = "wrong nof_phich_resources"
        self.assertEqual(cell['nof_phich_resources'], "1", msg=err)

    def _check_nof_prb(self, cell, expected_nof_prb):
        self.assertEqual(cell['nof_prb'], expected_nof_prb, msg="wrong nof_prb")

    def _check_nof_tx_ports(self, cell):
        self.assertEqual(cell['nof_tx_ports'], 1, msg="wrong nof_tx_ports")

    def _check_phich_len(self, cell):
        self.assertEqual(cell['phich_len'], "Normal", msg="wrong phich_len")

#    def test_6prb_t(self):
#        srcdir = os.getenv('srcdir') # set by gr testing framework
#        data_fname = os.path.join(srcdir, 'test_data/lte_frame_6prb_cellid_123')
#        fsrc = blocks.file_source(gr.sizeof_gr_complex, data_fname, repeat=True)
#        nsamps = int(1.92e6) # 1 sec of data
#        head = blocks.head(gr.sizeof_gr_complex, nsamps)
#        dltrig = downlink_trigger_c(self.psr_threshold, self.exit_on_success)
#        self.tb.connect(fsrc, head, dltrig)
#
#        # connect message passing interface
#        msgdebug = blocks.message_debug()
#        self.tb.msg_connect(dltrig, 'tracking_cell', msgdebug, 'store')
#
#        # run enobeb_data through ltetrigger
#        self.tb.run()
#
#        nmsgs = msgdebug.num_messages()
#        self.assertTrue(nmsgs, msg="No message triggered")
#
#        cell = pmt.to_python(msgdebug.get_message(0))
#        pprint(cell)
#
#        # test ltetrigger conditions
#        self._check_cell_id(cell, 123)
#        self._check_cp_len(cell)
#        self._check_nof_phich_resources(cell)
#        self._check_nof_prb(cell, 6)
#        self._check_nof_tx_ports(cell)
#        self._check_phich_len(cell)
#
#    def test_25prb_t(self):
#        srcdir = os.getenv('srcdir') # set by gr testing framework
#        data_fname = os.path.join(srcdir, 'test_data/lte_frame_25prb_cellid_124')
#        fsrc = blocks.file_source(gr.sizeof_gr_complex, data_fname, repeat=True)
#        srate = 7.68e6 # 7.68 MHz
#        head = blocks.head(gr.sizeof_gr_complex, int(srate)) # 1 sec of data
#        resamp_ratio = int(srate / REQUIRED_SAMPLE_RATE)
#        resampler = gr_filter.rational_resampler_ccc(1, resamp_ratio)
#        dltrig = downlink_trigger_c(self.psr_threshold, self.exit_on_success)
#        self.tb.connect(fsrc, head, resampler, dltrig)
#
#        # connect message passing interface
#        msgdebug = blocks.message_debug()
#        self.tb.msg_connect(dltrig, 'tracking_cell', msgdebug, 'store')
#
#        # run enobeb_data through ltetrigger
#        self.tb.run()
#
#        nmsgs = msgdebug.num_messages()
#        self.assertTrue(nmsgs, msg="No message triggered")
#
#        cell = pmt.to_python(msgdebug.get_message(0))
#        pprint(cell)
#
#        # test ltetrigger conditions
#        self._check_cell_id(cell, 124)
#        self._check_cp_len(cell)
#        self._check_nof_phich_resources(cell)
#        self._check_nof_prb(cell, 25)
#        self._check_nof_tx_ports(cell)
#        self._check_phich_len(cell)
#
#    def test_50prb_t(self):
#        srcdir = os.getenv('srcdir') # set by gr testing framework
#        data_fname = os.path.join(srcdir, 'test_data/lte_frame_50prb_cellid_125')
#        fsrc = blocks.file_source(gr.sizeof_gr_complex, data_fname, repeat=True)
#        srate = 15.36e6 # 15.36 MHz
#        head = blocks.head(gr.sizeof_gr_complex, int(srate)) # 1 sec of data
#        resamp_ratio = int(srate / REQUIRED_SAMPLE_RATE)
#        resampler = gr_filter.rational_resampler_ccc(1, resamp_ratio)
#        dltrig = downlink_trigger_c(self.psr_threshold, self.exit_on_success)
#        self.tb.connect(fsrc, head, resampler, dltrig)
#
#        # connect message passing interface
#        msgdebug = blocks.message_debug()
#        self.tb.msg_connect(dltrig, 'tracking_cell', msgdebug, 'store')
#
#        # run enobeb_data through ltetrigger
#        self.tb.run()
#
#        nmsgs = msgdebug.num_messages()
#        self.assertTrue(nmsgs, msg="No message triggered")
#
#        cell = pmt.to_python(msgdebug.get_message(0))
#        pprint(cell)
#
#        # test ltetrigger conditions
#        self._check_cell_id(cell, 125)
#        self._check_cp_len(cell)
#        self._check_nof_phich_resources(cell)
#        self._check_nof_prb(cell, 50)
#        self._check_nof_tx_ports(cell)
#        self._check_phich_len(cell)

    def test_100prb_t(self):
        srcdir = os.getenv('srcdir') # set by gr testing framework
        data_fname = os.path.join(srcdir, 'test_data/lte_frame_100prb_cellid_369')
        fsrc = blocks.file_source(gr.sizeof_gr_complex, data_fname, repeat=True)
        srate = 30.72e6 # 30.72 MHz
        head = blocks.head(gr.sizeof_gr_complex, int(srate)) # 1 sec of data
        resamp_ratio = int(srate / REQUIRED_SAMPLE_RATE)
        resampler = gr_filter.rational_resampler_ccc(1, resamp_ratio)
        dltrig = downlink_trigger_c(self.psr_threshold, self.exit_on_success)
        self.tb.connect(fsrc, head, resampler, dltrig)

        # connect message passing interface
        msgdebug = blocks.message_debug()
        self.tb.msg_connect(dltrig, 'tracking_cell', msgdebug, 'store')

        # run enobeb_data through ltetrigger
        self.tb.run()

        nmsgs = msgdebug.num_messages()
        self.assertTrue(nmsgs, msg="No message triggered")

        cell = pmt.to_python(msgdebug.get_message(0))
        pprint(cell)

        # test ltetrigger conditions
        self._check_cell_id(cell, 369)
        self._check_cp_len(cell)
        self._check_nof_phich_resources(cell)
        self._check_nof_prb(cell, 100)
        self._check_nof_tx_ports(cell)
        self._check_phich_len(cell)


if __name__ == '__main__':
    gr_unittest.run(qa_downlink_trigger_c, "qa_downlink_trigger_c.xml")
