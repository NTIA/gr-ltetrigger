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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <algorithm> /* copy */
#include <cstdio>    /* printf */
#include <stdexcept> /* runtime_error */

#include <gnuradio/io_signature.h>

#include "sss_impl.h"


namespace gr {
  namespace ltetrigger {

    // initialize static variables
    const pmt::pmt_t sss_impl::cell_id_tag_key = pmt::intern("cell_id");

    const pmt::pmt_t sss_impl::cp_type_tag_key = pmt::intern("cp_type");

    const pmt::pmt_t
    sss_impl::tracking_lost_tag_key = pmt::intern("tracking_lost");

    sss::sptr
    sss::make(int N_id_2)
    {
      return gnuradio::get_initial_sptr(new sss_impl {N_id_2});
    }

    /*
     * The private constructor
     */
    sss_impl::sss_impl(int N_id_2)
      : gr::sync_block("sss",
                       gr::io_signature::make(1, 1, sizeof(cf_t)),
                       gr::io_signature::make(1, 1, sizeof(cf_t))),
        d_N_id_2 {N_id_2}
    {
      srslte_use_standard_symbol_size(true);
      set_tag_propagation_policy(TPP_ALL_TO_ALL);

      if (srslte_sync_init(&d_sync, half_frame_length, max_offset, symbol_sz))
        throw std::runtime_error {"Error initializing SSS SYNC"};

      if (srslte_sync_set_N_id_2(&d_sync, N_id_2))
        throw std::runtime_error {"Error initializing SSS SYNC N_id_2"};

      if (srslte_sss_set_N_id_2(&d_sync.sss, N_id_2))
        throw std::runtime_error {"Error initializing SSS N_id_2"};

      set_output_multiple(half_frame_length);
    }

    /*
     * Our virtual destructor.
     */
    sss_impl::~sss_impl()
    {
      srslte_sync_free(&d_sync);
    }

    int
    sss_impl::work(int noutput_items,
                   gr_vector_const_void_star &input_items,
                   gr_vector_void_star &output_items)
    {
      const cf_t *in {static_cast<const cf_t *>(input_items[0])};
      cf_t *out {static_cast<cf_t *>(output_items[0])};

      get_tags_in_window(d_tracking_lost_tags, 0, 0, 1, tracking_lost_tag_key);

      if (!d_tracking_lost_tags.empty()) {
        srslte_sync_reset(&d_sync);
        d_subframe_idx = -1;
        d_tracking_lost_tags.clear();

        std::copy(in, &in[half_frame_length], out);
        return half_frame_length;
      }

      srslte_sync_t *sync {&d_sync};

      srslte_cp_t cp {srslte_sync_detect_cp(sync,
                                            const_cast<cf_t *>(in),
                                            slot_length)};

      srslte_sync_set_cp(sync, cp);

      unsigned int sss_idx {slot_length - 2 * symbol_sz - sync->cp_len};

      srslte_sss_m0m1_partial(&sync->sss,
                                    const_cast<cf_t *>(&in[sss_idx]),
                                    1, NULL,
                                    &sync->m0, &sync->m0_value,
                                    &sync->m1, &sync->m1_value);

      int retval = srslte_sss_N_id_1(&sync->sss, sync->m0, sync->m1);
      if (retval == SRSLTE_ERROR)
        return half_frame_length; // mib block ignores input with no tag
      else
        sync->N_id_1 = retval;

      int cell_id {srslte_sync_get_cell_id(sync)};

      //unsigned int subframe_idx {srslte_sss_subframe(sync->m0, sync->m1)};
      //if (d_subframe_idx < 0) {
      //  d_subframe_idx = subframe_idx;
      //} else {
      //  int expected_subframe_idx {(d_subframe_idx + 5) % 10};
      //  d_subframe_idx = subframe_idx;
      //  if (d_subframe_idx != expected_subframe_idx)
      //    std::printf("Expected subframe index %d, but got %d\n",
      //                expected_subframe_idx,
      //                d_subframe_idx);
      //}

      // TODO: consider using nitems_read to tag stream in pss and here
      //       so that we can see when there's been dropped frames

      const pmt::pmt_t cell_id_tag_value {pmt::from_long(cell_id)};
      add_item_tag(0, nitems_written(0), cell_id_tag_key, cell_id_tag_value);

      pmt::pmt_t cp_type_tag_value;
      if (SRSLTE_CP_ISNORM(sync->cp))
        cp_type_tag_value = pmt::PMT_T;
      else
        cp_type_tag_value = pmt::PMT_F;

      add_item_tag(0, nitems_written(0), cp_type_tag_key, cp_type_tag_value);

      std::copy(in, &in[half_frame_length], out);

      // Tell runtime system how many output items we produced.
      return half_frame_length;
    }

  } /* namespace ltetrigger */
} /* namespace gr */
