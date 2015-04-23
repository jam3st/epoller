#include "query.hpp"
#include "endians.hpp"
#include "logger.hpp"
#include "utils.hpp"


namespace Sb {
	namespace {
		constexpr size_t MAX_DOMAIN_LEN = 64;

		inline size_t getBits(const uint16_t& d, const size_t startBitPos, const size_t bitLen) {
			uint16_t mask = ((1u << bitLen) - 1) << startBitPos;
			return (d & mask) >> startBitPos;
		}

		inline size_t setBits(const uint16_t& d, const size_t startBitPos, const size_t bitLen, const uint16_t val) {
			uint16_t clearMask = ~(((1u << bitLen) - 1) << startBitPos);
			uint16_t setMask =  (val & ((1u << bitLen) - 1)) << startBitPos;
			return (d & clearMask) | setMask;
		}

		enum class Rcode {
			Ok = 0,
			FormatError = 1,
			ServerFailure = 2,
			NameError = 3,
			NotImplement = 4,
			Refused = 5
		};

		enum class Opcode {
			Query = 0,
			Iquery = 1,
			Status = 2
		};

		enum class Qr {
			Query = 0,
			Response = 1
		};

		class ReqFlags final {
			public:
				Qr qr() const {
					return Qr(getBits(d, 15, 1));
				}

				void qr(const Qr qr) {
					d = setBits(d, 15, 1, static_cast<decltype(d)>(qr));
				}

				Opcode opcode() const {
					return Opcode(getBits(d, 11, 4));
				}

				void opcode(const Opcode opcode) {
					d = setBits(d, 11, 4, static_cast<decltype(d)>(opcode));
				}

				bool aa() const {
					return bool(getBits(d, 10, 1));
				}

				void aa(const bool aa) {
					d = setBits(d, 10, 1, static_cast<decltype(d)>(aa));
				}

				bool tc() const {
					return bool(getBits(d, 9, 1));
				}

				void tc(const bool tc) {
					d = setBits(d, 9, 1, static_cast<decltype(d)>(tc));
				}

				bool rd() const {
					return bool(getBits(d, 8, 1));
				}

				void rd(const bool rd) {
					d = setBits(d, 8, 1, static_cast<decltype(d)>(rd));
				}

				bool ra() const {
					return bool(getBits(d, 7, 1));
				}

				void ra(const bool ra) {
					d = setBits(d, 7, 1, static_cast<decltype(d)>(ra));
				}

				size_t z() const {
					return bool(getBits(d, 6, 1));
				}

				void z(const size_t z) {
					d = setBits(d, 6, 1, static_cast<decltype(d)>(z));
				}

				bool ad() const {
					return bool(getBits(d, 5, 1));
				}

				void ad(const bool ad) {
					d = setBits(d, 5, 1, static_cast<decltype(d)>(ad));
				}

				bool cd() const {
					return bool(getBits(d, 4, 1));
				}

				void cd(const bool cd) {
					d = setBits(d, 4, 1, static_cast<decltype(d)>(cd));
				}

				Rcode rcode() const {
					return Rcode(getBits(d, 0, 4));
				}

				void rcode(const Rcode rcode) {
					d = setBits(d, 0	, 4, static_cast<decltype(d)>(rcode));
				}
			private:
				uint16_t d = 0;
		};

		struct Header final {
			uint16_t id;
			ReqFlags reqFlags;
			uint16_t qdcount;
			uint16_t ancount;
			uint16_t nscount;
			uint16_t arcount;
		};

		enum struct Qclass : uint16_t {
			Internet = 1
		};

		struct Footer final {
			Query::Qtype qtype;
			Qclass qclass;
		};

		struct Answer final {
			Query::Qtype qtype;
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

		Footer
		decodeFooter(const std::vector<uint8_t>& response, size_t& curPos) {
			Footer footer;
			footer.qtype = static_cast<Query::Qtype>(networkEndian(response[curPos], response[curPos + 1]));
			curPos = curPos + 2;
			footer.qclass = static_cast<Qclass>(networkEndian(response[curPos], response[curPos + 1]));
			curPos = curPos + 2;
			return footer;
		}

		Answer
		decodeAnswer(const std::vector<uint8_t>& response, size_t& curPos) {
			Answer answer;
			answer.qtype = static_cast<Query::Qtype>(networkEndian(response[curPos], response[curPos + 1]));
			curPos = curPos + 2;
			answer.qclass = static_cast<Qclass>(networkEndian(response[curPos], response[curPos + 1]));
			curPos = curPos + 2;
			answer.ttl = networkEndian(response[curPos], response[curPos + 1], response[curPos + 2], response[curPos + 3]);
			curPos = curPos + 4;
			answer.rdlength =  networkEndian(response[curPos], response[curPos + 1]);
			curPos = curPos + 2;
std::cerr << "ANWER " << (int)answer.qclass	<< " type " << (int)answer.qtype << " rdlen " << answer.rdlength << std::endl;
			return answer;
		}

		QueryHeader
		decodeHeader(const std::vector<uint8_t>& data) {
			QueryHeader header {};
			if(data.size() >= sizeof(header)) {
				for(size_t i = 0; i < sizeof(header) / sizeof(header.d[0]); ++i) {
					header.d[i] = networkEndian(data[i * 2], data[i * 2 + 1]);
				}
			}
std::cerr << "RCODE " << (int)header.h.reqFlags.rcode() << " TC " << header.h.reqFlags.tc() << " ra " << header.h.reqFlags.ra() << std::endl;
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
			std::string name;
			size_t len = response[curPos++];
			while(response[curPos] != '\0') {
				if((len & 0xC0) == 0xC0) {
					break;
				}
				if(len == 0) {
					name.push_back('.');
					len = response[curPos];
				} else {
					name.push_back(response[curPos]);
					len--;
				}
				curPos++;
			}
			curPos++;
			return name;
		}
	}

	namespace Query  {
		std::vector<uint8_t>
		resolve(uint16_t reqNo, const std::string& name, Qtype qType)  {
			QueryHeader header {};
			header.h.id = reqNo;
			header.h.reqFlags.qr(Qr::Query);
			header.h.reqFlags.opcode(Opcode::Query);
			header.h.reqFlags.tc(false);
			header.h.reqFlags.rd(true);
			header.h.reqFlags.ra(false);
			header.h.reqFlags.aa(false);
			header.h.qdcount = 1;
			QueryFooter footer {};
			footer.f.qclass = Qclass::Internet;
			footer.f.qtype = qType;
			std::vector<uint8_t> query {};
			query.reserve(sizeof(header) + sizeof(footer) + 2 * name.length());
			for(size_t i = 0; i < sizeof(header) / sizeof(header.d[0]); ++i) {
				networkEndian(header.d[i], query);
			}
			encodeName(name, query);
			for(size_t i = 0; i < sizeof(footer) / sizeof(footer.d[0]); ++i) {
				networkEndian(footer.d[i], query);
			}
			return query;
		}

		Qanswer
		decode(const std::vector<uint8_t>& data) {
			logDebug("decode begin" + toHexString(data) + "decode end");
			Qanswer reply;
			reply.valid = false;
			reply.timeStamp = SteadyClock::now();
			QueryHeader header = decodeHeader(data);
			auto curPos = sizeof(header);
			reply.reqNo = header.h.id;
			for(size_t i = header.h.qdcount; i > 0; --i) {
				reply.name = decodeName(data, curPos);
				Footer f = decodeFooter(data, curPos);
				(void)f;
			}
			for(size_t i = header.h.ancount; i > 0; --i) {
				(void)decodeName(data, curPos);
				Answer answer = decodeAnswer(data, curPos);
				reply.ttl = NanoSecs(answer.ttl * NanoSecsInSecs);
				reply.timeStamp = SteadyClock::now();
				if(answer.qclass == Qclass::Internet) {
					if(answer.rdlength == 4 && answer.qtype == Qtype::A) {
						const int prefixLen = 12;
						std::array<uint8_t, prefixLen> ip4Pref { {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff} };
						IpAddr addr;
						size_t i = 0;
						while(i < prefixLen) {
							addr[i] = ip4Pref[i];
							++i;
						}
						addr[i] = data[curPos];
						addr[i + 1] = data[curPos + 1];
						addr[i + 2] = data[curPos + 2];
						addr[i + 3] = data[curPos + 3];
						reply.addr.push_back(addr);
						reply.valid = true;
					} else if(answer.rdlength == 16 && answer.qtype == Qtype::Aaaa) {
						std::cerr << "IP6 ADDRESS IS " << networkEndian(data[curPos], data[curPos + 1], data[curPos + 2], data[curPos + 3]) << std::endl;
						reply.valid = true;
					}
				}
				curPos += answer.rdlength;
			}
			return reply;
		}
	};
}
