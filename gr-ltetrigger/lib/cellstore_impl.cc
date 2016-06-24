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

#include <gnuradio/io_signature.h>

#include "cellstore_impl.h"


namespace gr {
  namespace ltetrigger {

    const pmt::pmt_t cellstore_impl::track_port_id = pmt::intern("track");
    const pmt::pmt_t cellstore_impl::drop_port_id = pmt::intern("drop");
    const pmt::pmt_t cellstore_impl::cell_id_key = pmt::intern("cell_id");
    const pmt::pmt_t cellstore_impl::bad_cell_id_val = pmt::mp(1000);

    cellstore::sptr cellstore::make()
    {
      return gnuradio::get_initial_sptr (new cellstore_impl);
    }

    cellstore_impl::cellstore_impl()
      : block("cellstore",
              io_signature::make(0, 0, 0),
              io_signature::make(0, 0, 0))
    {
      message_port_register_in(track_port_id);
      set_msg_handler(track_port_id,
                      boost::bind(&cellstore_impl::track_cell, this, _1));

      message_port_register_in(drop_port_id);
      set_msg_handler(drop_port_id,
                      boost::bind(&cellstore_impl::drop_cell, this, _1));
    }

    bool cellstore_impl::tracking()
    {
      return is_tracking0() || is_tracking1() || is_tracking2();
    }

    std::vector<pmt::pmt_t> cellstore_impl::cells()
    {
      std::lock_guard<std::mutex> lock {d_mutex};

      return std::vector<pmt::pmt_t> {d_cell0, d_cell1, d_cell2};
    }

    void cellstore_impl::track_cell(pmt::pmt_t msg)
    {
      long cell_id {pmt::to_long(pmt::dict_ref(msg,
                                               cell_id_key,
                                               bad_cell_id_val))};

      if (cell_id == pmt::to_long(bad_cell_id_val))
        throw std::runtime_error {"Error tracking cell: bad message format"};

      auto N_id_2 = static_cast<unsigned char>(cell_id % 3);

      std::lock_guard<std::mutex> lock {d_mutex};

      switch (N_id_2) {
      case 0:
        assert(!is_tracking0());  // sanity check
        d_cell0 = msg;
        break;
      case 1:
        assert(!is_tracking1());  // sanity check
        d_cell1 = msg;
        break;
      case 2:
        assert(!is_tracking2());  // sanity check
        d_cell2 = msg;
        break;
      default:
        throw std::runtime_error {"track_cell switch hit default"};
      }
    }

    void cellstore_impl::drop_cell(pmt::pmt_t msg)
    {
      long cell_id {pmt::to_long(pmt::dict_ref(msg,
                                               cell_id_key,
                                               bad_cell_id_val))};

      if (cell_id == pmt::to_long(bad_cell_id_val))
        throw std::runtime_error {"Error dropping cell: bad message format"};

      auto N_id_2 = static_cast<unsigned char>(cell_id % 3);

      std::lock_guard<std::mutex> lock {d_mutex};

      switch (N_id_2) {
      case 0:
        assert(is_tracking0());  // sanity check
        d_cell0 = pmt::PMT_NIL;
        break;
      case 1:
        assert(is_tracking1());  // sanity check
        d_cell1 = pmt::PMT_NIL;
        break;
      case 2:
        assert(is_tracking2());  // sanity check
        d_cell2 = pmt::PMT_NIL;
        break;
      default:
        throw std::runtime_error {"drop_cell switch hit default"};
      }
    }

  } /* namespace ltetrigger */
} /* namespace gr */
