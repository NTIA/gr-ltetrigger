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

import ltetrigger


class downlink_trigger_c(gr.hier_block2):
    """
    Hierarchical block for LTE downlink detection based on srsLTE

    This block requires input sampled at (or resampled to) 1.92 MHz
    """
    def __init__(self, psr_threshold, exit_on_success=False):
        gr.hier_block2.__init__(self,
                                "downlink_trigger_c",
                                gr.io_signature(1, 1, gr.sizeof_gr_complex),
                                gr.io_signature(0, 0, 0))

        self.psr_threshold = psr_threshold
        self.exit_on_success = exit_on_success

        self.pss0 = ltetrigger.pss(N_id_2=0, psr_threshold=self.psr_threshold)
        self.pss1 = ltetrigger.pss(N_id_2=1, psr_threshold=self.psr_threshold)
        self.pss2 = ltetrigger.pss(N_id_2=2, psr_threshold=self.psr_threshold)
        self.sss0 = ltetrigger.sss(N_id_2=0)
        self.sss1 = ltetrigger.sss(N_id_2=1)
        self.sss2 = ltetrigger.sss(N_id_2=2)
        self.mib0 = ltetrigger.mib(exit_on_success=self.exit_on_success)
        self.mib1 = ltetrigger.mib(exit_on_success=self.exit_on_success)
        self.mib2 = ltetrigger.mib(exit_on_success=self.exit_on_success)
        self.tag0 = blocks.tag_debug(gr.sizeof_gr_complex, "mib0")
        self.tag1 = blocks.tag_debug(gr.sizeof_gr_complex, "mib1")
        self.tag2 = blocks.tag_debug(gr.sizeof_gr_complex, "mib2")
        self.tag0.set_display(False)
        self.tag1.set_display(False)
        self.tag2.set_display(False)

        self.connect(self, self.pss0, self.sss0, self.mib0, self.tag0)
        self.connect(self, self.pss1, self.sss1, self.mib1, self.tag1)
        self.connect(self, self.pss2, self.sss2, self.mib2, self.tag2)

        pss_drop_port_id = "pss_drop"
        self.msg_connect(self.pss0, pss_drop_port_id,
                         self.mib0, pss_drop_port_id)
        self.msg_connect(self.pss1, pss_drop_port_id,
                         self.mib1, pss_drop_port_id)
        self.msg_connect(self.pss2, pss_drop_port_id,
                         self.mib2, pss_drop_port_id)

        drop_port_id = "drop"
        self.message_port_register_hier_in(drop_port_id)
        self.message_port_register_hier_out(drop_port_id)

        self.msg_connect(self.mib0, drop_port_id,
                         self, drop_port_id)
        self.msg_connect(self.mib1, drop_port_id,
                         self, drop_port_id)
        self.msg_connect(self.mib2, drop_port_id,
                         self, drop_port_id)

        track_port_id = "track"
        self.message_port_register_hier_in(track_port_id)
        self.message_port_register_hier_out(track_port_id)

        self.msg_connect(self.mib0, track_port_id,
                         self, track_port_id)
        self.msg_connect(self.mib1, track_port_id,
                         self, track_port_id)
        self.msg_connect(self.mib2, track_port_id,
                         self, track_port_id)

    def set_psr_threshold(self, threshold):
        self.pss0.set_psr_threshold(threshold)
        self.pss1.set_psr_threshold(threshold)
        self.pss2.set_psr_threshold(threshold)
