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
       * psr_threshold: peak-to-side-lobe ratio threshold above which pass If
       *                the PSS correlates above `psr_threshold`, a point is
       *                added to the tracking score or tracking status is
       *                confirmed if already tracking.  If the PSS correlates
       *                below `psr_threshold`, a point is deducted from the
       *                tracking score and a resync is forced. If the cell was
       *                tracking but the tracking score gets decremented to 0,
       *                the block signals "tracking lost".
       * track_after: enter tracking state after n correlations in a row above
       *              threshold.
       * track_every: if tracking, check correlation of every n half-frames
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
                       int track_after=16,
                       int track_every=8);

      /*! \brief Return maximum peak to side-lobe ratio seen by this block */
      virtual float max_psr() const = 0;

      /*! \brief Return mean peak to side-lobe ratio seen in 200 frames */
      virtual float mean_psr() const = 0;

      /*! \brief Return mean carrier frequency offset seen by this block */
      virtual float mean_cfo() const = 0;

      /*! \brief Set peak to side-lobe threshold */
      virtual void set_psr_threshold(float threshold) = 0;

      /*! \brief Return current peak to side-lobe ratio threshold */
      virtual float psr_threshold() const = 0;

      /*! \brief Return current tracking score */
      virtual float tracking_score() const = 0;
    };

  } // namespace ltetrigger
} // namespace gr

#endif /* INCLUDED_LTETRIGGER_PSS_H */
