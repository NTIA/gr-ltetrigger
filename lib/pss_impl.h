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

#ifndef INCLUDED_LTETRIGGER_PSS_IMPL_H
#define INCLUDED_LTETRIGGER_PSS_IMPL_H

#include <pmt/pmt.h>

#include <srslte/srslte.h>

#include <ltetrigger/pss.h>


const int moving_avg_sz {200};


namespace gr {
  namespace ltetrigger {

    class pss_impl : public pss
    {
    private:

      struct tracking_t
      {
        explicit operator bool() const { return is_tracking; }
        void start() { is_tracking = true; }
        void stop() { is_tracking = false; }
        int score {0};
        int timer {0}; // skip frame sync when tracking and timer > 0
      private:
        bool is_tracking {false};
      } d_tracking;

      static const int slot_length {960};
      static const int half_frame_length {10 * slot_length};
      static const int full_frame_length {2 * half_frame_length};
      static const int symbol_sz {128};

      static const pmt::pmt_t tracking_lost_tag_key;

      void incr_score(tracking_t &tracking);
      void reset_score(tracking_t &tracking);

      bool d_tracking_lost {false};

      float compute_moving_avg(const float data[], size_t npts) const;

      srslte_pss_t d_pss;
      float d_psr_data[moving_avg_sz] {0.0};
      size_t d_psr_i {0};
      float d_psr {0};
      float d_psr_max {0.0};
      int d_peak_pos {0};

      srslte_cfo_t d_cfo;
      cf_t d_channel_estimation_buffer[SRSLTE_PSS_LEN];
      float d_cfo_data[moving_avg_sz] {0.0};
      size_t d_cfo_i {0};

      int d_N_id_2;
      float d_psr_threshold;
      int d_track_after_n_frames;
      int d_track_every_n_frames;

    public:
      pss_impl(int N_id_2,
               float psr_threshold,
               int track_after,
               int track_every);
      ~pss_impl();

      int general_work(int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items);

      float max_psr() const { return d_psr_max; }
      float mean_psr() const { return compute_moving_avg(d_psr_data, d_psr_i); }
      float mean_cfo() const { return compute_moving_avg(d_cfo_data, d_cfo_i); }
      void set_psr_threshold(float threshold) { d_psr_threshold = threshold; }
      float psr_threshold() const { return d_psr_threshold; }
      float tracking_score() const { return d_tracking.score; }
    };

  } // namespace ltetrigger
} // namespace gr

#endif /* INCLUDED_LTETRIGGER_PSS_IMPL_H */
