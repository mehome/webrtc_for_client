# This file is generated by gyp; do not edit.

export builddir_name ?= ./webrtc/out
.PHONY: all
all:
	$(MAKE) -C .. rtc_event_log_proto rtc_event_log_parser rtc_event_log rtc_event_log2rtp_dump webrtc_nonparallel_tests webrtc_nonparallel_tests_run xmllite_xmpp_unittests rtc_unittests rtc_unittests_run webrtc video_quality_test webrtc_perf_tests webrtc_perf_tests_run video_replay video_loopback video_engine_tests webrtc_tests video_engine_tests_run screenshare_loopback
