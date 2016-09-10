/* -*- c++ -*- */
/*
 * Copyright 2016 Institute for Telecommunication Sciences.
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

#include <list>
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

      std::mutex d_mutex;

      void track_cell(pmt::pmt_t msg);
      void drop_cell(pmt::pmt_t msg);

      std::list<pmt::pmt_t> d_cells;

    public:
      cellstore_impl();
      //~cellstore_impl();

      bool tracking();
      std::vector<pmt::pmt_t> cells();
      pmt::pmt_t latest_cell();
};

  } // namespace ltetrigger
} // namespace gr

#endif /* INCLUDED_LTETRIGGER_CELLSTORE_IMPL_H */
