/*
*
*
*  Copyright by ztgame@2017
*  Written by lfz 
*
*
*/

#ifndef RTC_FEC_H
#define RTC_FEC_H

#include <vector>
#include <memory>
#include <string.h>
#include "webrtc/base/basictypes.h"
#include "webrtc/base/byteorder.h"
#include <webrtc/base/logging.h>
#include "webrtc/base/criticalsection.h"
#include "webrtc/base/platform_thread.h"
#include "webrtc/system_wrappers/include/sleep.h"
#include "webrtc/system_wrappers/include/event_wrapper.h"

extern "C" {
#include "third_party/openfec/src/lib_common/of_openfec_api.h"
} 

using namespace std;
using namespace rtc;
using namespace webrtc;

const uint32_t kFecSourceNum = 10;
const uint32_t kFecRepairNum = 5;
const uint32_t kFecMaxLength = 1200;

struct FecHeader {
	uint8_t		version;
	uint8_t 	pt;
	uint16_t 	seq_num;
	uint32_t	info;
	uint32_t	ssrc;
};

class RtcFecEncoderCallback
{
  public:
	virtual int SendPacket(const uint8_t *data, const uint32_t size) = 0;
	
  protected:
  	virtual ~RtcFecEncoderCallback() { }
};

class RtcFecEncoder
{
  public:
	RtcFecEncoder(RtcFecEncoderCallback *callback)
		: received_num(0),
		current_seq(0),
		last_encoded_timestamp(0),
		session(nullptr),
		packet_pointer(nullptr),
		callback_(callback) { }
	
	~RtcFecEncoder();

	static RtcFecEncoder* Create(const uint32_t source_num, 
								 const uint32_t repair_num,
								 const uint32_t max_length,
								 RtcFecEncoderCallback *callback);
	
	int Init(const uint32_t source_num, const uint32_t repair_num, 
			 const uint32_t max_length);
	
	bool InsertPacket(const uint8_t *data, const uint32_t size);
	//int GetFecPacket(const uint32_t index, uint8_t **data, uint32_t &size);
	
  private:
	int CreateFecInstance(const uint32_t source_num,
						  const uint32_t repair_num, 
			 			  const uint32_t max_length);
	void InitFecHeader(uint8_t *buffer, const uint16_t seq_num,
							 const uint32_t ssrc, const uint32_t timestamp);
	inline uint32_t MaxSize() { 
		return param.encoding_symbol_length + sizeof(FecHeader); 
	}
	inline uint32_t Columns() { return param.nb_source_symbols; }
	inline uint32_t Rows() { return param.nb_source_symbols + param.nb_repair_symbols; }

	uint16_t received_num;
	uint16_t current_seq;
	uint32_t last_encoded_timestamp;
  	of_session_t *session;
	of_rs_2_m_parameters_t param;
	
	uint8_t **packet_pointer;
	RtcFecEncoderCallback *callback_;
	std::vector<std::unique_ptr<uint8_t>> packet_list; 
};


class RtcFecDecoderCallback
{
  public:
	virtual int ReceivePacket(const uint8_t *data, const uint32_t size) = 0;
	
  protected:
  	virtual ~RtcFecDecoderCallback() { }
};

class RtcFecDecoder
{
  public:
	struct FecPacket {
		FecPacket() 
			: status(0),
			timestamp(0),
			frame_begin(false),
			frame_end(false),
			size(0) {}

		uint32_t 	status;
		uint32_t	timestamp;
		bool		frame_begin;
		bool		frame_end;
		uint8_t		data[1500];  // should be larger than MaxSize()
		uint32_t	size;
	};
	
	RtcFecDecoder(RtcFecDecoderCallback *callback)
		: last_decoded_timestamp(0),
		fec_pointer(nullptr),
		raw_pointer(nullptr),
		session(nullptr),
		callback_(callback),
		event_(EventWrapper::Create()),
		worker(new PlatformThread(Runnable, this, "FecDecoder")),
		lock(new CriticalSection()){ }
	
	~RtcFecDecoder();

	static RtcFecDecoder* Create(const uint32_t source_num,
								 const uint32_t repair_num,
						  		 const uint32_t max_length,
						  		 RtcFecDecoderCallback *callback);
	
	int Init(const uint32_t source_num, const uint32_t repair_num, 
			 const uint32_t max_length);

	static bool Runnable(void *obj);
	int InsertFecData(const uint8_t *data, const uint32_t size);

  private:
  	void Process();
	int DecodedFec();
	uint32_t FindEarliestPacket();
	int ResetDecoder(const uint32_t index);
	int ResetFecPacket(const uint32_t index);
	int Decoded(const uint32_t index);
	int CreateFecInstance();
	void DeliverPacket();
	std::vector<FecPacket*> GetPacketsInFrame(const uint32_t index,
											  const uint32_t timestamp);

  	inline uint32_t MaxSize() { 
		return param.encoding_symbol_length + sizeof(FecHeader); 
	}
	inline uint32_t MinSize() { return sizeof(FecHeader) + sizeof(uint32_t); }
	inline uint32_t Rows() { return (param.nb_source_symbols + param.nb_repair_symbols); }
	inline uint32_t Columns() { return param.nb_source_symbols; }
	inline bool Source(const uint32_t index) { return (index % Rows()) < Columns(); }
	
	enum { kInserted = 0x1, kDecoded = 0x10, kDelivered = 0x100, kReleased = 0x111 };
	
	uint32_t last_decoded_timestamp;
	
	uint8_t** fec_pointer;
	uint8_t** raw_pointer;
  	of_session_t *session;
	of_rs_2_m_parameters_t param;

	RtcFecDecoderCallback *callback_;
	std::unique_ptr<EventWrapper> event_;
	std::unique_ptr<PlatformThread> worker;
	std::unique_ptr<CriticalSection> lock;
	std::vector<std::unique_ptr<FecPacket>> packet_list;
};

#endif
