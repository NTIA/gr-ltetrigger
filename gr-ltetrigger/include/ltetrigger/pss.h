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


#ifndef INCLUDED_LTETRIGGER_PSS_H
#define INCLUDED_LTETRIGGER_PSS_H

#include <ltetrigger/api.h>
#include <gnuradio/block.h>


namespace gr {
  namespace ltetrigger {

    /*!
     * \brief Primary synchronization
     * \ingroup ltetrigger
     *
     */
    class LTETRIGGER_API pss : virtual public gr::block
    {
    public:
      typedef boost::shared_ptr<pss> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of ltetrigger::pss.
       *
       * N_id_2: initialize block to search for this N_id_2
       * psr_threshold: peak-to-side-lobe ratio threshold above which pass
       *                through frame for further processing
       * track_after: Enter tracking state after n correlations in a row above
       *              threshold. A single correlation below threshold drops
       *              out of tracking
       * track_every: In tracking state, check correlation of every n
       *              half-frames
       *
       * NOTE: track_every should be large enough so that follow on block sss
       *       has enough frames to correlate before potentially being passed
       *       a disjoint set of frames
       *
       * To avoid accidental use of raw pointers, ltetrigger::pss's
       * constructor is in a private implementation
       * class. ltetrigger::pss::make is the public interface for
       * creating new instances.
       */
      static sptr make(int N_id_2,
                       float psr_threshold,
                       int track_after=32,
                       int track_every=4);
    };

  } // namespace ltetrigger
} // namespace gr

#endif /* INCLUDED_LTETRIGGER_PSS_H */
