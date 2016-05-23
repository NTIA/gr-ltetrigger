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
//#include <cstdio>    /* printf */
#include <stdexcept> /* runtime_error */

#include <gnuradio/io_signature.h>

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
      srslte_use_standard_symbol_size(true);
      //srslte_verbose = SRSLTE_VERBOSE_DEBUG;

      if (srslte_pss_synch_init(&d_pss, half_frame_length))
        throw std::runtime_error("Error initializing PSS");

      if (srslte_pss_synch_set_N_id_2(&d_pss, N_id_2))
        throw std::runtime_error("Error initializing PSS N_id_2");

      if (srslte_cfo_init(&d_cfo, symbol_sz))
        throw std::runtime_error("Error initializing CFO");

      set_history(half_frame_length);
      set_output_multiple(half_frame_length);
      message_port_register_out(tracking_port_id);
    }

    /*
     * virtual destructor
     */
    pss_impl::~pss_impl()
    {
      srslte_pss_synch_free(&d_pss);

      printf("N_id_2 [%d]: avg PSR: %f, max PSR: %f\n",
             d_N_id_2,
             d_psr_sum / d_psr_nsummed,
             d_psr_max);
    }

    void
    pss_impl::incr_score(tracking_t &tracking)
    {
      int max_score = d_track_after_n_frames;

      //if (d_N_id_2 == 1) printf("%d ", tracking.score[d_N_id_2]);

      if (tracking.id[d_N_id_2] && tracking.score[d_N_id_2] == max_score)
        // nothing to do
        return;

      tracking.score[d_N_id_2]++;

      if (!tracking.id[d_N_id_2] && tracking.score[d_N_id_2] == max_score)
        tracking.id[d_N_id_2] = true;
    }

    void
    pss_impl::decr_score(tracking_t &tracking)
    {
      //if (d_N_id_2 == 1) printf("DEBUG: dec tracking score: %d\n", tracking.score[d_N_id_2]);

      if (!tracking.score[d_N_id_2])
        return;

      tracking.score[d_N_id_2]--;

      if (tracking.id[d_N_id_2])
        tracking.countdown[d_N_id_2] = 0; // force resync

      if (tracking.score[d_N_id_2] == 0) {
        // signal cell dropped
        d_tracking.id[d_N_id_2] = false;
        d_psr_max = 0;
        message_port_pub(tracking_port_id, pmt::PMT_NIL);
      }
    }

    int
    pss_impl::general_work(int noutput_items,
                           gr_vector_int &ninput_items,
                           gr_vector_const_void_star &input_items,
                           gr_vector_void_star &output_items)
    {
      const cf_t *in = &(static_cast<const cf_t *>(input_items[0])[history()-1]);
      cf_t *out = static_cast<cf_t *>(output_items[0]); // output stream

      if (!d_tracking.any() || d_tracking.countdown[d_N_id_2] == 0) {
        d_tracking.countdown[d_N_id_2] = d_track_every_n_frames;
        d_peak_pos = srslte_pss_synch_find_pss(&d_pss,
                                               const_cast<cf_t *>(in),
                                               &d_psr);

        // track avg PSR
        d_psr_sum += d_psr;
        d_psr_nsummed++;
      } else {
        d_tracking.countdown[d_N_id_2]--;
      }

      if (d_psr > d_psr_threshold)
        incr_score(d_tracking);
      else
        decr_score(d_tracking);

      if (d_psr > d_psr_max)
        d_psr_max = d_psr;

      if (d_tracking.id[d_N_id_2]) {
        int frame_start = d_peak_pos - slot_length;
        d_peak_pos = slot_length;

        noutput_items = half_frame_length;
        int nconsume = frame_start + noutput_items;

        assert(nconsume < ninput_items[0]);

        std::copy(&in[frame_start], &in[nconsume], out);

        consume_each(nconsume);

        // estimate CFO
        float cfo = srslte_pss_synch_cfo_compute(&d_pss,
                                                 &out[slot_length - symbol_sz]);
        d_cfo_mean = SRSLTE_VEC_CMA(cfo, d_cfo_mean, nitems_written(0));

        // correct CFO in place
        srslte_cfo_correct(&d_cfo, out, out, -d_cfo_mean / symbol_sz);

        if (srslte_pss_synch_chest(&d_pss,
                                   &out[slot_length - symbol_sz],
                                   d_channel_estimation_buffer))
          throw std::runtime_error("Error computing channel estimation");

      } else {
        // nothing to see, move along
        consume_each(half_frame_length); // drop the current half frame
        noutput_items = 0;               // don't propogate anything
      }

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace ltetrigger */
} /* namespace gr */
