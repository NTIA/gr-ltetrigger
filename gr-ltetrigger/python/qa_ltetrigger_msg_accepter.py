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
import time

from gnuradio import gr, gr_unittest
from gnuradio import blocks

import pmt

from ltetrigger_msg_accepter import ltetrigger_msg_accepter


def create_cell_pmt(cell):
    d = pmt.make_dict()
    for key, val in cell.iteritems():
        k = pmt.intern(key)
        if type(val) is str:
            v = pmt.intern(val)
        elif type(val) is int or type(val) is long:
            v = pmt.from_uint64(val)
            d = pmt.dict_add(d, k, v)

    return d


def create_fc_pmt(fc):
    f = pmt.from_uint64(int(fc))
    return f


class qa_ltetrigger_msg_accepter(gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block()
        self.msg_accepter = ltetrigger_msg_accepter()

    def tearDown(self):
        self.tb = None

    def test_001_t(self):
        # set up fg

        fc1 = int(2145e6) # 2145 MHz
        cell1 = {'nports': 1, 'nprb': 6, 'cell_id': 369, 'link_type': 'downlink'}

        fc2 = int(736e6) # 736 MHz
        cell2 = {'nports': 1, 'nprb': 6, 'cell_id': 200, 'link_type': 'downlink'}

        fc3 = int(1700e6) # 1700 MHz
        cell3 = {'nports': 1, 'nprb': 6, 'cell_id': 80, 'link_type': 'downlink'}

        expected_result = defaultdict(dict)

        fc_port = pmt.intern('fc')
        register_cell_port = pmt.intern('cell_detected')
        drop_cell_port = pmt.intern('cell_lost')

        # add cell1
        expected_result[fc1][cell1['cell_id']] = cell1

        self.msg_accepter.set_fc(create_fc_pmt(fc1))
        self.msg_accepter.record_cell(create_cell_pmt(cell1))

        self.assertEqual(self.msg_accepter.fc, fc1)
        self.assertEqual(self.msg_accepter.cells, expected_result)

        # add cell2
        expected_result[fc2][cell2['cell_id']] = cell2

        self.msg_accepter.set_fc(create_fc_pmt(fc2))
        self.msg_accepter.record_cell(create_cell_pmt(cell2))

        self.assertEqual(self.msg_accepter.fc, fc2)
        self.assertEqual(self.msg_accepter.cells, expected_result)

        # add cell3
        expected_result[fc3][cell3['cell_id']] = cell3

        self.msg_accepter.set_fc(create_fc_pmt(fc3))
        self.msg_accepter.record_cell(create_cell_pmt(cell3))

        self.assertEqual(self.msg_accepter.fc, fc3)
        self.assertEqual(self.msg_accepter.cells, expected_result)

        # drop cell3
        expected_result[fc3].pop(cell3['cell_id'])

        self.msg_accepter.set_fc(create_fc_pmt(fc3))
        self.msg_accepter.drop_cell(create_cell_pmt(cell3))

        self.assertEqual(self.msg_accepter.fc, fc3)
        self.assertEqual(self.msg_accepter.cells, expected_result)

        # drop cell2
        expected_result[fc2].pop(cell2['cell_id'])

        self.msg_accepter.set_fc(create_fc_pmt(fc2))
        self.msg_accepter.drop_cell(create_cell_pmt(cell2))

        self.assertEqual(self.msg_accepter.fc, fc2)
        self.assertEqual(self.msg_accepter.cells, expected_result)

        # drop cell1
        expected_result[fc1].pop(cell1['cell_id'])

        self.msg_accepter.set_fc(create_fc_pmt(fc1))
        self.msg_accepter.drop_cell(create_cell_pmt(cell1))

        self.assertEqual(self.msg_accepter.fc, fc1)
        self.assertEqual(self.msg_accepter.cells, expected_result)


if __name__ == '__main__':
    #import pdb; pdb.set_trace()
    gr_unittest.run(qa_ltetrigger_msg_accepter, "qa_ltetrigger_msg_accepter.xml")