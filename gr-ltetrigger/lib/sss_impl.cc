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
#include <cstdio>    /* printf */
#include <cstdlib>   /* exit, EXIT_FAILURE */

#include <gnuradio/io_signature.h>

#include "sss_impl.h"


namespace gr {
  namespace ltetrigger {

    sss::sptr
    sss::make(int N_id_2)
    {
      return gnuradio::get_initial_sptr(new sss_impl(N_id_2));
    }

    /*
     * The private constructor
     */
    sss_impl::sss_impl(int N_id_2)
      : gr::sync_block("sss",
                       gr::io_signature::make(1, 1, sizeof(cf_t)),
                       gr::io_signature::make(1, 1, sizeof(cf_t))),
        d_N_id_2(N_id_2)
    {
      if (srslte_pss_synch_init(&d_sss[N_id_2], half_frame_length)) {
        std::cerr << "Error initializing SSS object" << std::endl;
        exit(EXIT_FAILURE);
      }
      if (srslte_pss_synch_set_N_id_2(&d_sss[N_id_2], N_id_2)) {
        std::cerr << "Error initializing N_id_2" << std::endl;
        exit(EXIT_FAILURE);
      }

      set_output_multiple(half_frame_length);
    }

    /*
     * Our virtual destructor.
     */
    sss_impl::~sss_impl()
    {
    }

    int
    sss_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const cf_t *in = static_cast<const cf_t *>(input_items[0]);
      cf_t *out = static_cast<cf_t *>(output_items[0]);

      // Do <+signal processing+>

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace ltetrigger */
} /* namespace gr */
