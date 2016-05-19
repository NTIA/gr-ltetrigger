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


from gnuradio import gr, blocks
import gnuradio.filter as gr_filter

import ltetrigger


class downlink_trigger_c(gr.hier_block2):
    """
    Hierarchical block for LTE downlink detection based on srsLTE

    This block requires input sampled at (or resampled to) 1.92 MHz
    """
    def __init__(self, psr_threshold=4.5):
        gr.hier_block2.__init__(self,
                                "downlink_trigger_c",
                                gr.io_signature(1, 1, gr.sizeof_gr_complex),
                                gr.io_signature(0, 0, 0))

        self.psr_threshold = psr_threshold

        self.cfo = ltetrigger.cfo()
        self.pss0 = ltetrigger.pss(N_id_2=0, psr_threshold=self.psr_threshold)
        self.pss1 = ltetrigger.pss(N_id_2=1, psr_threshold=self.psr_threshold)
        self.pss2 = ltetrigger.pss(N_id_2=2, psr_threshold=self.psr_threshold)
        self.sss0 = ltetrigger.sss(N_id_2=0)
        self.sss1 = ltetrigger.sss(N_id_2=1)
        self.sss2 = ltetrigger.sss(N_id_2=2)
        #self.mib0 = ltetrigger.mib()
        #self.mib1 = ltetrigger.mib()
        #self.mib2 = ltetrigger.mib()
        self.tag0 = blocks.tag_debug(gr.sizeof_gr_complex, "mib0")
        self.tag1 = blocks.tag_debug(gr.sizeof_gr_complex, "mib1")
        self.tag2 = blocks.tag_debug(gr.sizeof_gr_complex, "mib2")
        self.tag0.set_display(True)
        self.tag1.set_display(True)
        self.tag2.set_display(True)


        # TODO: insert CFO block

        self.connect(self, self.pss0, self.sss0, self.tag0)
        self.connect(self, self.pss1, self.sss1, self.tag1)
        self.connect(self, self.pss2, self.sss2, self.tag2)
