#include "query.hpp"
#include <iostream>

namespace Sb {
	namespace {
		const size_t MAX_DOMAIN_LEN = 64;

		union U16Swap {
			uint16_t d16;
			uint8_t b[sizeof(d16)];
		};

		union U32Swap {
			uint32_t d32;
			uint8_t b[sizeof(d32)];
		};

		const bool bigEndian = [] () {
			return U16Swap { 0xb000 }.b[0] == 0xb0;
		};

		enum struct Qtype : uint16_t {
			A = 1,
			Ns = 2,
			Cname = 5,
			Soa = 6,
			Ptr = 12,
			Mx = 15,
			Txt = 16,
			Aaaa = 28,
			Srv = 33,
			Ds = 43,
			Rrsig  = 46,
			Dnskey = 48,
			Nsec3 =	50,
			Nsec3Param = 51,
			Tlsa = 52,
			Any = 255
		};

		struct Header final {
			uint16_t id;
			bool rd : 1;
			bool tc : 1;
			bool aa : 1;
			enum struct opcode {
				Query = 0,
				Iquery = 1,
				Status = 2
			} opcode  : 4 ;
			enum class qr {
				Query = 0,
				Response = 1
			} qr : 1;
			bool ra : 1;
			uint8_t z :	3;
			enum class rcode {
				Ok = 0,
				FormatError = 1,
				ServerFailure = 2,
				NameError = 3,
				NotImplement = 4,
				Refused = 5
			} rcode  : 4 ;
			uint16_t qdcount;
			uint16_t ancount;
			uint16_t nscount;
			uint16_t arcount;
		};

		enum struct Qclass : uint8_t {
			Internet = 1
		};

		struct Footer final {
			Qtype qtype;
			Qclass qclass;
		};

		struct Answer final {
			Qtype qtype;
			Qclass qclass;
			uint32_t ttl;
			uint16_t rdlength;
		};

		union QueryHeader {
			Header h;
			uint16_t d[sizeof(Header) / sizeof(uint16_t)];
		};

		union QueryFooter {
			Footer f;
			uint16_t d[sizeof(Footer) / sizeof(uint16_t)];
		};


		inline size_t
		getLen(const std::string& name, size_t& prevDotPos)
		{
			size_t dotPos = name.find('.', prevDotPos + 1);
			size_t len {};
			if(dotPos == std::string::npos) {
				len = name.length() - prevDotPos - 1;
			} else if(prevDotPos == 0) {
				len = dotPos - prevDotPos;
			} else {
				len = dotPos - prevDotPos - 1;
			}
			prevDotPos = dotPos;
			if(len >= MAX_DOMAIN_LEN) {
				len = MAX_DOMAIN_LEN - 1;
			}
			return len;
		}

		inline
		uint16_t
		swapper(const uint8_t d0, const uint8_t d1) {
			U16Swap tmp {};
			if(bigEndian) {
				tmp.b[1] = d0;
				tmp.b[0] = d1;
			} else {
				tmp.b[0] = d0;
				tmp.b[1] = d1;
			}
			return tmp.d16;
		}

		inline
		void
		swapper(const uint16_t d, std::vector<uint8_t>& stream) {
			const U16Swap tmp { d };
			if(bigEndian) {
				stream.push_back(tmp.b[1]);
				stream.push_back(tmp.b[0]);
			} else {
				stream.push_back(tmp.b[0]);
				stream.push_back(tmp.b[1]);
			}
		}

		inline
		uint32_t
		swapper(const uint8_t d0, const uint8_t d1 ,const uint8_t d2, const uint8_t d3) {
			U32Swap tmp {};
			if(bigEndian) {
				tmp.b[3] = d0;
				tmp.b[2] = d1;
				tmp.b[1] = d2;
				tmp.b[0] = d3;
			} else {
				tmp.b[0] = d0;
				tmp.b[1] = d1;
				tmp.b[2] = d2;
				tmp.b[3] = d3;
			}
			return tmp.d32;
		}

//		inline
//		void
//		swapper(const uint16_t d, std::vector<uint8_t>& stream) {
//			const U32Swap tmp { d };
//			if(bigEndian) {
//				stream.push_back(tmp.b[3]);
//				stream.push_back(tmp.b[2]);
//				stream.push_back(tmp.b[1]);
//				stream.push_back(tmp.b[0]);
//			} else {
//				stream.push_back(tmp.b[0]);
//				stream.push_back(tmp.b[1]);
//				stream.push_back(tmp.b[2]);
//				stream.push_back(tmp.b[3]);
//			}
//		}

		Footer
		decodeFooter(const std::vector<uint8_t>& response, size_t& curPos) {
			Footer footer {};
			footer.qtype = static_cast<Qtype>(swapper(response[curPos], response[curPos + 1]));
			curPos = curPos + 2;
			footer.qclass = static_cast<Qclass>(swapper(response[curPos], response[curPos + 1]));
			curPos = curPos + 2;
			return footer;
		}

		Answer
		decodeAnswer(const std::vector<uint8_t>& response, size_t& curPos) {
			Answer answer {};
			answer.qtype = static_cast<Qtype>(swapper(response[curPos], response[curPos + 1]));
			curPos = curPos + 2;
			answer.qclass = static_cast<Qclass>(swapper(response[curPos], response[curPos + 1]));
			curPos = curPos + 2;
			answer.ttl = swapper(response[curPos], response[curPos + 1], response[curPos + 2], response[curPos + 3]);
			curPos = curPos + 4;
			answer.rdlength =  swapper(response[curPos], response[curPos + 1]);
			curPos = curPos + 2;
			return answer;
		}

		QueryHeader
		decodeHeader(const std::vector<uint8_t>& data) {
			QueryHeader header {};
			if(data.size() >= sizeof(header)) {
				for(size_t i = 0; i < sizeof(header) / sizeof(header.d[0]); ++i) {
					header.d[i] = swapper(data[i * 2], data[i * 2 + 1]);
				}
			}
			return header;
		}

		void
		encodeName(const std::string& name, std::vector<uint8_t>& query)
		{
			size_t prevDotPos = 0;
			size_t len = getLen(name, prevDotPos);
			query.push_back(len);
			for(auto it = name.begin(); it != name.end(); ++it) {
				if(*it == '.') {
					len = getLen(name, prevDotPos);
					query.push_back(len);
				} else if(len !=  std::string::npos) {
					query.push_back(*it);
					if(len == 0) {
						len = std::string::npos;
					} else {
						--len;
					}
				}
			}
			query.push_back('\0');
		}

		std::string
		decodeName(const std::vector<uint8_t>& response, size_t& curPos) {
			while(response[curPos] != '\0') {
				if((response[curPos] & 0xC0) == 0xC0) {
					curPos++;
					break;
				}
				curPos++;
			}
			curPos++;
			return "blah";
		}
	}

	namespace Query {

		std::vector<uint8_t>
		resolve(const std::string& name) {
			QueryHeader header {};
			header.h.id = 0x3412;
			header.h.qr = Header::qr::Query;
			header.h.opcode = Header::opcode::Query;
			header.h.rd = true;
			header.h.qdcount = 1;
			QueryFooter footer {};
			footer.f.qclass = Qclass::Internet;
			footer.f.qtype = Qtype::A;
			std::vector<uint8_t> query {};
			query.reserve(sizeof(header) + sizeof(footer) + 2 * name.length());
			for(size_t i = 0; i < sizeof(header) / sizeof(header.d[0]); ++i) {
				swapper(header.d[i], query);
			}
			encodeName(name, query);
			for(size_t i = 0; i < sizeof(footer) / sizeof(footer.d[0]); ++i) {
				swapper(footer.d[i], query);
			}
			return query;
		}

		Qanswer
		decode(const std::vector<uint8_t>& data) {
			Qanswer reply;
			reply.timeStamp = SteadyClock::now();
			QueryHeader header = decodeHeader(data);
			auto curPos = sizeof(header);
			for(size_t i = header.h.qdcount; i > 0; --i) {
				auto name = decodeName(data, curPos);
				Footer f = decodeFooter(data, curPos);
				(void)name;
				(void)f;
			}
			for(size_t i = header.h.ancount; i > 0; --i) {
				auto name = decodeName(data, curPos);
				Answer answer = decodeAnswer(data, curPos);
				reply.ttl = NanoSecs(answer.ttl * NanoSecsInSecs);
				reply.timeStamp = SteadyClock::now();
				if(answer.qclass == Qclass::Internet && answer.qtype == Qtype::A) {
					if(answer.rdlength == 4) {
						std::cerr << "IP ADDRESS IS " << swapper(data[curPos], data[curPos + 1], data[curPos + 2], data[curPos + 3]) << std::endl;
					}
				}
			}
			return reply;
		}
	};
}
