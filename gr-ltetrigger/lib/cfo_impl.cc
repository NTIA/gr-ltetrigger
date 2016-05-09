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
#include <iostream> /* cerr */

#include <gnuradio/io_signature.h>
#include "cfo_impl.h"


namespace gr {
  namespace ltetrigger {

    cfo::sptr
    cfo::make()
    {
      return gnuradio::get_initial_sptr(new cfo_impl());
    }

    /*
     * private constructor
     */
    cfo_impl::cfo_impl()
      : gr::sync_block("cfo",
                       gr::io_signature::make(<+MIN_IN+>, <+MAX_IN+>, sizeof(<+ITYPE+>)),
                       gr::io_signature::make(<+MIN_OUT+>, <+MAX_OUT+>, sizeof(<+OTYPE+>)))
    {
      if (srslte_cfo_init(&cfocorr, frame_length)) {
        std::cerr << "Error initiating CFO" << std::endl;
        return -1;
      }

      set_output_multiple(full_frame_length); // 2 half-frames
    }

    /*
     * virtual destructor.
     */
    cfo_impl::~cfo_impl()
    {
      srslte_cfo_free(cfocorr);
    }

    int
    cfo_impl::work(int noutput_items,
                   gr_vector_const_void_star &input_items,
                   gr_vector_void_star &output_items)
    {
      const cf_t *in = static_cast<const cf_t *>(input_items[0]);
      cf_t *out = static_cast<cf_t *>(output_items[0]);

      if (is_enabled()) {
        srslte_cfo_correct(cfocorr, in, out, d_fc); // correct first half frame
        srslte_cfo_correct(cfocorr,                 // correct second half frame
                           &in[half_frame_length],
                           &out[half_frame_length],
                           d_fc)
      } else {
        std::copy(in, &in[full_frame_length], out);
      }

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace ltetrigger */
} /* namespace gr */
