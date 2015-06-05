#pragma once
#include <byteswap.h>

namespace Sb {
      namespace {
            union U16Swap {
                  uint16_t d16;
                  uint8_t b[sizeof(d16)];
            };

            union U32Swap {
                  uint32_t d32;
                  uint8_t b[sizeof(d32)];
            };

            const bool bigEndian = []() {
                  return U16Swap { 0xb000 } .b[0] == 0xb0;
            };

            inline uint16_t networkEndian(const uint16_t d0) {
                  if(bigEndian) {
                        return bswap_16(d0);
                  } else {
                        return d0;
                  }
            }

            inline uint32_t networkEndian(const uint32_t d0) {
                  if(bigEndian) {
                        return bswap_32(d0);
                  } else {
                        return d0;
                  }
            }

            inline uint64_t networkEndian(const uint64_t d0) {
                  if(bigEndian) {
                        return bswap_64(d0);
                  } else {
                        return d0;
                  }
            }

            inline uint16_t networkEndian(const uint8_t d0, const uint8_t d1) {
                  U16Swap tmp { };

                  if(bigEndian) {
                        tmp.b[1] = d0;
                        tmp.b[0] = d1;
                  } else {
                        tmp.b[0] = d0;
                        tmp.b[1] = d1;
                  }

                  return tmp.d16;
            }

            inline void networkEndian(uint16_t const d, std::vector<uint8_t>& stream) {
                  const U16Swap tmp{d};
                  if(bigEndian) {
                        stream.push_back(tmp.b[1]);
                        stream.push_back(tmp.b[0]);
                  } else {
                        stream.push_back(tmp.b[0]);
                        stream.push_back(tmp.b[1]);
                  }
            }

            inline void networkEndian(uint32_t const d, std::vector<uint8_t>& stream) {
                  const U32Swap tmp{d};
                  if (bigEndian) {
                        stream.push_back(tmp.b[3]);
                        stream.push_back(tmp.b[2]);
                        stream.push_back(tmp.b[1]);
                        stream.push_back(tmp.b[0]);
                  } else {
                        stream.push_back(tmp.b[0]);
                        stream.push_back(tmp.b[1]);
                        stream.push_back(tmp.b[2]);
                        stream.push_back(tmp.b[3]);
                  }
            }


            inline uint32_t networkEndian(const uint8_t d0, const uint8_t d1, const uint8_t d2, const uint8_t d3) {
                  U32Swap tmp { };

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
      }
}
