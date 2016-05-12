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

#include <algorithm> /* copy */
#include <cassert>   /* assert */
#include <cstdint>   /* uint64_t */
#include <cstdio>    /* printf */
#include <cstdlib>   /* exit, EXIT_FAILURE */

#include <gnuradio/io_signature.h>
//#include <gnuradio/thread/thread.h> /* gr::thread */

#include <pmt/pmt.h> /* pmt_t */

#include "pss_impl.h"


namespace gr {
  namespace ltetrigger {

    // initialize static variables
    pss_impl::tracking_t pss_impl::d_tracking;

    pss::sptr
    pss::make(int N_id_2, float psr_threshold, int track_after, int track_every)
    {
      return gnuradio::get_initial_sptr(new pss_impl(N_id_2,
                                                     psr_threshold,
                                                     track_after,
                                                     track_every));
    }
    /*
     * private constructor
     */
    pss_impl::pss_impl(int N_id_2,
                       float psr_threshold,
                       int track_after,
                       int track_every)
      : gr::block("pss",
                  gr::io_signature::make(1, 1, sizeof(cf_t)),
                  gr::io_signature::make(1, 1, sizeof(cf_t))),
        d_N_id_2(N_id_2),
        d_psr_threshold(psr_threshold),
        d_track_after_n_frames(track_after),
        d_track_every_n_frames(track_every)
    {
      if (srslte_pss_synch_init(&d_pss[N_id_2], half_frame_length)) {
        std::cerr << "Error initializing PSS object" << std::endl;
        exit(EXIT_FAILURE);
      }
      if (srslte_pss_synch_set_N_id_2(&d_pss[N_id_2], N_id_2)) {
        std::cerr << "Error initializing N_id_2" << std::endl;
        exit(EXIT_FAILURE);
      }

      set_output_multiple(full_frame_length);
    }

    /*
     * virtual destructor
     */
    pss_impl::~pss_impl()
    {
      srslte_pss_synch_free(&d_pss[d_N_id_2]);
    }

    void
    pss_impl::incr_score(tracking_t &tracking)
    {
      int max_score = d_track_after_n_frames;

      if (tracking.N_id_2[d_N_id_2] && tracking.score[d_N_id_2] == max_score)
        // nothing to do
        return;

      tracking.score[d_N_id_2]++;

      if (!tracking.N_id_2[d_N_id_2] && tracking.score[d_N_id_2] == max_score)
        tracking.N_id_2[d_N_id_2] = true;
    }

    void
    pss_impl::decr_score(tracking_t &tracking)
    {
      if (!tracking.score[d_N_id_2])
        return;

      tracking.score[d_N_id_2]--;

      if (tracking.N_id_2[d_N_id_2]) {
        tracking.N_id_2[d_N_id_2] = false;
        printf("Lost tracking on N_id_2 %d\n", d_N_id_2);
      }
    }

    int
    pss_impl::general_work(int noutput_items,
                           gr_vector_int &ninput_items,
                           gr_vector_const_void_star &input_items,
                           gr_vector_void_star &output_items)
    {
      const cf_t *in = static_cast<const cf_t *>(input_items[0]); // input stream
      cf_t *out = static_cast<cf_t *>(output_items[0]); // output stream

      if (!d_tracking.any() || !d_tracking.count[d_N_id_2]) {
        d_tracking.count[d_N_id_2] = d_track_every_n_frames;
        d_peak_pos[d_N_id_2] = srslte_pss_synch_find_pss(&d_pss[d_N_id_2],
                                                         const_cast<cf_t *>(in),
                                                         &d_psr[d_N_id_2]);
      } else {
        d_tracking.count[d_N_id_2]--;
      }

      if (d_psr[d_N_id_2] > d_psr_threshold) {
        incr_score(d_tracking);
      } else {
        decr_score(d_tracking);
      }

      if (d_tracking.N_id_2[d_N_id_2]) {
        // track max PSR for fun
        if (d_psr[d_N_id_2] > d_max_psr[d_N_id_2]) {
          d_max_psr[d_N_id_2] = d_psr[d_N_id_2];
          printf("New max PSR value for N_id_2 %d: %f\n",
                 d_N_id_2,
                 d_max_psr[d_N_id_2]);
        }

        int frame_start = d_peak_pos[d_N_id_2] - slot_length;
        if (frame_start < 0)
          frame_start += half_frame_length;

        noutput_items = half_frame_length;
        int nconsume = frame_start + noutput_items;

        assert(nconsume < ninput_items[0]);

        std::copy(&in[frame_start], &in[nconsume], out);

        consume_each(nconsume);

      } else {
        // reset max PSR if tracked cell is long gone
        if (d_tracking.score[d_N_id_2] == 0)
          d_max_psr[d_N_id_2] = 0;

        // nothing to see, move along
        consume_each(half_frame_length); // drop the current half frame
        noutput_items = 0;               // don't propogate anything
      }

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace ltetrigger */
} /* namespace gr */
