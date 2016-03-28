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

from collections import defaultdict

import numpy
from gnuradio import gr
import pmt

class ltetrigger_msg_accepter(gr.basic_block):
    """
    docstring for block ltetrigger_msg_accepter
    """
    def __init__(self):
        gr.basic_block.__init__(self,
                                name="ltetrigger_msg_accepter",
                                in_sig=None,
                                out_sig=None)

        self.fc = 0
        self.linktype = 'downlink'

        # Map which can store 1 cell per cell_id per center frequency
        #
        # Example:
        # >>> cell = {'nports': 1, 'nprb': 6, 'cell_id': 369, 'link_type': 'downlink'}
        # >>> fc = 2145
        # >>> cells[fc][cell['cell_id']] = cell
        # >>> cells.keys()
        # [2145]
        # >>> cells[2145].keys()
        # [369]
        # >>> cells[2145][369]
        # {'nports': 1, 'nprb': 6, 'cell_id': 369, 'link_type': 'downlink'}
        self.cells = defaultdict(dict)

        self.required_keys = set(('link_type', 'cell_id', 'nprb', 'nports'))

        self.message_port_register_in(pmt.intern('fc'))
        self.set_msg_handler(pmt.intern('fc'), self.set_fc)

        self.message_port_register_in(pmt.intern('cell_detected'))
        self.set_msg_handler(pmt.intern('cell_detected'), self.record_cell)

        self.message_port_register_in(pmt.intern('cell_lost'))
        self.set_msg_handler(pmt.intern('cell_lost'), self.drop_cell)

    def set_fc(self, msg):
        self.fc = int(pmt.to_python(msg))

    def set_linktype_downlink(self):
        self.linktype = 'downlink'

    def set_linktype_uplink(self):
        self.linktype = 'uplink'

    def record_cell(self, msg):
        cell = pmt.to_python(msg)
        self._check_cell(cell)
        self.cells[self.fc][cell['cell_id']] = cell
        self.cells[self.fc][cell['cell_id']].update({'link_type': self.linktype})

    def drop_cell(self, msg):
        cell = pmt.to_python(msg)
        self._check_cell(cell)
        self.cells[self.fc].pop(cell['cell_id'])
        #if not cells[fc]:
        #    cells.pop(fc)

    def _check_cell(self, cell):
        if not self.required_keys.issuperset(cell.keys()):
            err = "{!r} not a subset of required keys: {!r}"
            raise TypeError(err.format(cell.keys(), self.required_keys))
