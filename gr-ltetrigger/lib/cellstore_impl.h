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

#ifndef INCLUDED_LTETRIGGER_CELLSTORE_IMPL_H
#define INCLUDED_LTETRIGGER_CELLSTORE_IMPL_H

#include <vector>
#include <mutex>

#include <ltetrigger/cellstore.h>


namespace gr {
  namespace ltetrigger {

    class cellstore_impl : public cellstore
    {
    private:
      static const pmt::pmt_t track_port_id;
      static const pmt::pmt_t drop_port_id;
      static const pmt::pmt_t cell_id_key;
      static const pmt::pmt_t bad_cell_id_val;

      bool is_tracking0() { return !pmt::is_null(d_cell0); }
      bool is_tracking1() { return !pmt::is_null(d_cell1); }
      bool is_tracking2() { return !pmt::is_null(d_cell2); }

      std::mutex d_mutex;

      pmt::pmt_t d_cell0 {pmt::PMT_NIL};
      pmt::pmt_t d_cell1 {pmt::PMT_NIL};
      pmt::pmt_t d_cell2 {pmt::PMT_NIL};

      void track_cell(pmt::pmt_t msg);
      void drop_cell(pmt::pmt_t msg);

    public:
      cellstore_impl();
      //~cellstore_impl();

      bool tracking();
      std::vector<pmt::pmt_t> cells();
};

  } // namespace ltetrigger
} // namespace gr

#endif /* INCLUDED_LTETRIGGER_CELLSTORE_IMPL_H */
