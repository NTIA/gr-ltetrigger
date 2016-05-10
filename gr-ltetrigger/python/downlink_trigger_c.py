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


REQUIRED_SAMPLE_RATE = 1.92e6 # 1.92 MHz


class downlink_trigger_c(gr.hier_block2):
    """
    Hierarchical block for LTE downlink detection based on srsLTE
    """
    def __init__(self, sample_rate):
        gr.hier_block2.__init__(self,
                                "downlink_trigger_c",
                                gr.io_signature(1, 1, gr.sizeof_gr_complex),
                                gr.io_signature(0, 0, 0))

        resamp_ratio = float(sample_rate) / float(REQUIRED_SAMPLE_RATE)
        self.resampler = gr_filter.fractional_resampler_cc(0, resamp_ratio)
        self.cfo = ltetrigger.cfo()
        self.pss = ltetrigger.pss()
        #self.sss = ltetrigger.sss()
        self.tag = blocks.tag_debug(gr.sizeof_gr_complex, "pss")
        self.tag.set_display(True)

        if resamp_ratio == 1:
            self.connect(self, self.pss, self.tag)
        else:
            self.connect(self, self.resampler, self.cfo, self.pss, self.tag)
