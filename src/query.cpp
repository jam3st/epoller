#include "query.hpp"
#include "endians.hpp"
#include "logger.hpp"
#include <iostream>

namespace Sb {
	namespace {
		constexpr size_t MAX_DOMAIN_LEN = 64;

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
			Footer footer {};
			footer.qtype = static_cast<Query::Qtype>(swapEndian(response[curPos], response[curPos + 1]));
			curPos = curPos + 2;
			footer.qclass = static_cast<Qclass>(swapEndian(response[curPos], response[curPos + 1]));
			curPos = curPos + 2;
			return footer;
		}

		Answer
		decodeAnswer(const std::vector<uint8_t>& response, size_t& curPos) {
			Answer answer {};
			answer.qtype = static_cast<Query::Qtype>(swapEndian(response[curPos], response[curPos + 1]));
			curPos = curPos + 2;
			answer.qclass = static_cast<Qclass>(swapEndian(response[curPos], response[curPos + 1]));
			curPos = curPos + 2;
			answer.ttl = swapEndian(response[curPos], response[curPos + 1], response[curPos + 2], response[curPos + 3]);
			curPos = curPos + 4;
			answer.rdlength =  swapEndian(response[curPos], response[curPos + 1]);
			curPos = curPos + 2;
std::cerr << "ANWER " << (int)answer.qclass	<< " type " << (int)answer.qtype << " rdlen " << answer.rdlength << std::endl;
			return answer;
		}

		QueryHeader
		decodeHeader(const std::vector<uint8_t>& data) {
			QueryHeader header {};
			if(data.size() >= sizeof(header)) {
				for(size_t i = 0; i < sizeof(header) / sizeof(header.d[0]); ++i) {
					header.d[i] = swapEndian(data[i * 2], data[i * 2 + 1]);
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
			std::string name;
			size_t len = response[curPos++];
std::cerr << " len " << len << std::endl;
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
			std::cerr << "name is " <<  name << std::endl;
			return name;
		}
	}

	namespace Query  {
		std::vector<uint8_t>
		resolve(uint16_t reqNo, const std::string& name, Qtype qType)  {
			QueryHeader header {};
			header.h.id = reqNo;
			header.h.qr = Header::qr::Query;
			header.h.opcode = Header::opcode::Query;
			header.h.rd = true;
			header.h.qdcount = 1;
			QueryFooter footer {};
			footer.f.qclass = Qclass::Internet;
			footer.f.qtype = qType;
			std::vector<uint8_t> query {};
			query.reserve(sizeof(header) + sizeof(footer) + 2 * name.length());
			for(size_t i = 0; i < sizeof(header) / sizeof(header.d[0]); ++i) {
				swapEndian(header.d[i], query);
			}
			encodeName(name, query);
			for(size_t i = 0; i < sizeof(footer) / sizeof(footer.d[0]); ++i) {
				swapEndian(footer.d[i], query);
			}
			return query;
		}

		Qanswer
		decode(const std::vector<uint8_t>& data) {
			Qanswer reply;
			reply.valid = false;
			reply.timeStamp = SteadyClock::now();
			QueryHeader header = decodeHeader(data);
			auto curPos = sizeof(header);
			reply.reqNo = header.h.id;
			for(size_t i = header.h.qdcount; i > 0; --i) {
				reply.name = decodeName(data, curPos);
				Footer f = decodeFooter(data, curPos);
std::cerr << "Reply: " << std::to_string(i) << std::endl;
				(void)f;
			}
	std::cerr << "Counts: " << std::to_string(header.h.arcount) << std::endl;
			for(size_t i = header.h.ancount; i > 0; --i) {
				(void)decodeName(data, curPos);
				Answer answer = decodeAnswer(data, curPos);
				reply.ttl = NanoSecs(answer.ttl * NanoSecsInSecs);
std::cerr << "Reply: " << std::to_string(i) << std::endl;
std::cerr << reply.name << " TTL " << reply.ttl.count() << " CLASS " << std::to_string((int)answer.qclass) << std::endl;
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
InetDest dest { addr, 12444 };
logDebug("resolved added " + dest.toString());

						reply.valid = true;
					} else if(answer.rdlength == 16 && answer.qtype == Qtype::Aaaa) {
						std::cerr << "IP6 ADDRESS IS " << swapEndian(data[curPos], data[curPos + 1], data[curPos + 2], data[curPos + 3]) << std::endl;
						reply.valid = true;
					}
				}
			}
			return reply;
		}
	};
}
