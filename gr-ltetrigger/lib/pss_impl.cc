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
#include <cstdlib>   /* exit, EXIT_FAILURE */
#include <future>    /* future, async */
#include <cstdint>   /* uint64_t */

#include <gnuradio/io_signature.h>
#include <gnuradio/thread/thread.h> /* gr::thread */

#include "pss_impl.h"


namespace gr {
  namespace ltetrigger {

    pss::sptr
    pss::make()
    {
      return gnuradio::get_initial_sptr
        (new pss_impl());
    }

    /*
     * private constructor
     */
    pss_impl::pss_impl()
      : gr::block("pss",
                  gr::io_signature::make(1, 1, sizeof(cf_t)),
                  gr::io_signature::make(1, 1, sizeof(cf_t)))
    {
      for (int i = 0; i < 3; i++) {
        if (srslte_pss_synch_init(&d_pss[i], half_frame_length)) {
          std::cerr << "Error initializing PSS object" << std::endl;
          exit(EXIT_FAILURE);
        }
        if (srslte_pss_synch_set_N_id_2(&d_pss[i], i)) {
          std::cerr << "Error initializing N_id_2" << std::endl;
          exit(EXIT_FAILURE);
        }
      }
    }

    /*
     * virtual destructor
     */
    pss_impl::~pss_impl()
    {
      for (int i = 0; i < 3; i++) {
        srslte_pss_synch_free(&d_pss[i]);
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

      // parallelize peak search for 3 values of N_id_2
      for (int i = 0; i < 3; i++)
        d_peak_pos[i] = std::async(std::launch::async,
                                   [&, i](){ return srslte_pss_synch_find_pss(&d_pss[i],
                                                                              const_cast<cf_t *>(in),
                                                                              &d_peak_value[i]); });

      // wait for results
      for (int i = 0; i < 3; i++)
        d_peak_pos[i].wait();

      // find the value of N_id_2 with the highest peak correlation value
      float max_value = -1;
      for (int i = 0; i < 3; i++) {
        if (d_peak_value[i] > max_value) {
          max_value = d_peak_value[i];
          d_N_id_2 = i;
        }
      }

      if (max_value > d_peak_threshold) {
        // tag and ship
        int rel_offset = d_peak_pos[d_N_id_2].get();
        uint64_t abs_offset = nitems_written(0) + rel_offset;

        noutput_items = half_frame_length;
        int nconsume = rel_offset + noutput_items;

        assert(nconsume < ninput_items[0]);

        // add PDU length tag
        add_item_tag(0,
                     abs_offset,
                     pmt::mp(length_tag_key),
                     pmt::mp(half_frame_length));


        // add N_id_2 tag
        add_item_tag(0,
                     abs_offset,
                     pmt::mp(N_id_2_tag_key),
                     pmt::mp(d_N_id_2));

        std::copy(&in[rel_offset], &in[nconsume], out);

        consume_each(nconsume);

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
