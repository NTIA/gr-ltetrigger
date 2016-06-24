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


#ifndef INCLUDED_LTETRIGGER_CELLSTORE_H
#define INCLUDED_LTETRIGGER_CELLSTORE_H

#include <pmt/pmt.h>

#include <ltetrigger/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace ltetrigger {

    /*!
     * \brief Stores any cell currently being tracked by downlink_trigger_c
     * \ingroup ltetrigger
     *
     * \details
     * This block can store up to 3 tracked cells, one for each N_id_2
     * (0, 1, 2).
     */
    class LTETRIGGER_API cellstore : virtual public gr::block
    {
    public:
      typedef boost::shared_ptr<cellstore> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of ltetrigger::cellstore.
       *
       * To avoid accidental use of raw pointers, ltetrigger::cellstore's
       * constructor is in a private implementation
       * class. ltetrigger::cellstore::make is the public interface for
       * creating new instances.
       */
      static sptr make();

      /*! \brief Return true if tracking any cells, else false */
      virtual bool tracking() = 0;

      /*! \brief Return std::vector of 3 PMTs
       *
       * \details
       * `ltetrigger` can simultaneously track 3 cells (one for each N_id_2).
       * This function returns a vector of 3 PMTs, where the vector index
       * corresponds to the N_id_2 of the tracked cell. The value is PMT_NIL
       * if no cell is currently being tracked, and a PMT dict type of cell
       * information if a cell is being tracked.
       */
      virtual std::vector<pmt::pmt_t> cells() = 0;
    };

  } // namespace ltetrigger
} // namespace gr

#endif /* INCLUDED_LTETRIGGER_CELLSTORE_H */
