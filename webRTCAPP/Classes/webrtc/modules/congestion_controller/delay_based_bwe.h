/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_CONGESTION_CONTROLLER_DELAY_BASED_BWE_H_
#define WEBRTC_MODULES_CONGESTION_CONTROLLER_DELAY_BASED_BWE_H_

#include <memory>
#include <utility>
#include <vector>

#include "webrtc/base/checks.h"
#include "webrtc/base/constructormagic.h"
#include "webrtc/base/race_checker.h"
#include "webrtc/modules/congestion_controller/median_slope_estimator.h"
#include "webrtc/modules/congestion_controller/probe_bitrate_estimator.h"
#include "webrtc/modules/congestion_controller/trendline_estimator.h"
#include "webrtc/modules/remote_bitrate_estimator/aimd_rate_control.h"
#include "webrtc/modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"
#include "webrtc/modules/remote_bitrate_estimator/inter_arrival.h"
#include "webrtc/modules/remote_bitrate_estimator/overuse_detector.h"
#include "webrtc/modules/remote_bitrate_estimator/overuse_estimator.h"

namespace webrtc {

class RtcEventLog;

class DelayBasedBwe {
 public:
  static const int64_t kStreamTimeOutMs = 2000;

  struct Result {
    Result() : updated(false), probe(false), target_bitrate_bps(0) {}
    Result(bool probe, uint32_t target_bitrate_bps)
        : updated(true), probe(probe), target_bitrate_bps(target_bitrate_bps) {}
    bool updated;
    bool probe;
    uint32_t target_bitrate_bps;
  };

  DelayBasedBwe(RtcEventLog* event_log, const Clock* clock);
  virtual ~DelayBasedBwe();

  Result IncomingPacketFeedbackVector(
      const std::vector<PacketFeedback>& packet_feedback_vector,
      rtc::Optional<uint32_t> acked_bitrate_bps);
  void OnRttUpdate(int64_t avg_rtt_ms, int64_t max_rtt_ms);
  bool LatestEstimate(std::vector<uint32_t>* ssrcs,
                      uint32_t* bitrate_bps) const;
  void SetStartBitrate(int start_bitrate_bps);
  void SetMinBitrate(int min_bitrate_bps);
  int64_t GetExpectedBwePeriodMs() const;

 private:
  void IncomingPacketFeedback(const PacketFeedback& packet_feedback);
  Result OnLongFeedbackDelay(int64_t arrival_time_ms);

  Result MaybeUpdateEstimate(bool overusing,
                             rtc::Optional<uint32_t> acked_bitrate_bps);
  // Updates the current remote rate estimate and returns true if a valid
  // estimate exists.
  bool UpdateEstimate(int64_t now_ms,
                      rtc::Optional<uint32_t> acked_bitrate_bps,
                      bool overusing,
                      uint32_t* target_bitrate_bps);

  rtc::RaceChecker network_race_;
  RtcEventLog* const event_log_;
  const Clock* const clock_;
  std::unique_ptr<InterArrival> inter_arrival_;
  std::unique_ptr<TrendlineEstimator> trendline_estimator_;
  OveruseDetector detector_;
  int64_t last_seen_packet_ms_;
  bool uma_recorded_;
  AimdRateControl rate_control_;
  ProbeBitrateEstimator probe_bitrate_estimator_;
  size_t trendline_window_size_;
  double trendline_smoothing_coeff_;
  double trendline_threshold_gain_;
  int consecutive_delayed_feedbacks_;
  uint32_t last_logged_bitrate_;
  BandwidthUsage last_logged_state_;
  bool in_sparse_update_experiment_;

  RTC_DISALLOW_IMPLICIT_CONSTRUCTORS(DelayBasedBwe);
};

}  // namespace webrtc

#endif  // WEBRTC_MODULES_CONGESTION_CONTROLLER_DELAY_BASED_BWE_H_
