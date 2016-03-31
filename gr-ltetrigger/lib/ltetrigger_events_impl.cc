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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cassert>
#include <iostream>

#include <gnuradio/io_signature.h>
#include "ltetrigger_events_impl.h"

namespace gr {
  namespace ltetrigger {

    ltetrigger_events::sptr
    ltetrigger_events::make()
    {
      return gnuradio::get_initial_sptr
        (new ltetrigger_events_impl());
    }

    /*
     * The private constructor
     */
    ltetrigger_events_impl::ltetrigger_events_impl()
      : gr::block("ltetrigger_events",
                  gr::io_signature::make(0, 0, 0),
                  gr::io_signature::make(0, 0, 0)),
        d_fc(0), d_linktype("downlink")

    {
      message_port_register_in(set_fc_port);
      set_msg_handler(set_fc_port,
                      boost::bind(&ltetrigger_events_impl::set_fc, this, _1));

      message_port_register_in(record_cell_port);
      set_msg_handler(record_cell_port,
                      boost::bind(&ltetrigger_events_impl::record_cell, this, _1));

      message_port_register_in(drop_cell_port);
      set_msg_handler(drop_cell_port,
                      boost::bind(&ltetrigger_events_impl::drop_cell, this, _1));

    }

    void
    ltetrigger_events_impl::set_fc(pmt::pmt_t f)
    {
      gr::thread::scoped_lock guard(d_mutex);

      assert(pmt::is_number(f));
      // TODO: think a bit more academically about possiblity of overflow here
      d_fc = static_cast<size_t>(pmt::to_float(f));
    }

    void
    ltetrigger_events_impl::record_cell(pmt::pmt_t cell)
    {

      gr::thread::scoped_lock guard(d_mutex);

      assert(is_valid(cell));

      pmt::pmt_t cell_id_pmt = pmt::dict_ref(cell, pmt::mp("cell_id"), pmt::mp(-1));
      size_t cell_id = static_cast<size_t>(pmt::to_uint64(cell_id_pmt));
      d_cells[d_fc][cell_id] = cell;
    }

    void
    ltetrigger_events_impl::drop_cell(const pmt::pmt_t &cell)
    {
      gr::thread::scoped_lock guard(d_mutex);

      assert(is_valid(cell));

      pmt::pmt_t cell_id_pmt = pmt::dict_ref(cell, pmt::mp("cell_id"), pmt::mp(-1));
      size_t cell_id = static_cast<size_t>(pmt::to_uint64(cell_id_pmt));
      d_cells[d_fc].erase(cell_id);
    }

    bool
    ltetrigger_events_impl::is_valid(const pmt::pmt_t &cell)
    {
      bool cell_is_valid = false;

      if (pmt::is_dict(cell)) {
        cell_is_valid = true;

        for (pmt::pmt_t key : required_cell_keys) {
          if (!pmt::dict_has_key(cell, key)) {
            cell_is_valid = false;
            break;
          }
        }
      }

      return cell_is_valid;
    }

    void
    ltetrigger_events_impl::print_cells()
    {
      std::cout << "Cells:" << std::endl;

      for (auto& freqmap : d_cells) {
        // freqmap is a std::pair of frequencies and <cell_id, cell_dict> pairs
        std::cout << "  " << freqmap.first << " MHz" << std::endl;
        for (auto& cellmap : freqmap.second) {
          std::cout << "\t" << cellmap.first << "\t" << cellmap.second << std::endl;
        }
      }
    }


  } /* namespace ltetrigger */
} /* namespace gr */
