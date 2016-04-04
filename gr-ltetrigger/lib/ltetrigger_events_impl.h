/* -*- c++ -*- */
/*
 * Copyright 2016 <+YOU OR YOUR COMPANY+>.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_LTETRIGGER_LTETRIGGER_EVENTS_IMPL_H
#define INCLUDED_LTETRIGGER_LTETRIGGER_EVENTS_IMPL_H

#include <unordered_map>
#include <vector>

#include <gnuradio/thread/thread.h>
#include <pmt/pmt.h>

#include <ltetrigger/ltetrigger_events.h>

namespace gr {
  namespace ltetrigger {

    class ltetrigger_events_impl : public ltetrigger_events
    {
    private:
      bool is_valid(const pmt::pmt_t &cell);
      void print_cells();

      const pmt::pmt_t set_fc_port = pmt::mp("fc");
      const pmt::pmt_t record_cell_port = pmt::mp("cell_detected");
      const pmt::pmt_t drop_cell_port = pmt::mp("cell_lost");

      const std::vector<pmt::pmt_t> required_cell_keys = {pmt::mp("linktype"),
                                                          pmt::mp("cell_id"),
                                                          pmt::mp("nprb"),
                                                          pmt::mp("nports")};

      gr::thread::mutex d_mutex;
      size_t d_fc;
      std::string d_linktype;
      ltetrigger_events::celldb_t d_cells;

    public:
      ltetrigger_events_impl();

      size_t fc() {return d_fc;};
      void set_fc(const pmt::pmt_t &f);
      std::string linktype() {return d_linktype;};
      void set_linktype_downlink() {d_linktype = "downlink";};
      void set_linktype_uplink() {d_linktype = "uplink";};
      const ltetrigger_events::celldb_t& cells();
      void record_cell(pmt::pmt_t cell);
      void drop_cell(const pmt::pmt_t &cell);

    };

  } // namespace ltetrigger
} // namespace gr

#endif /* INCLUDED_LTETRIGGER_LTETRIGGER_EVENTS_IMPL_H */
