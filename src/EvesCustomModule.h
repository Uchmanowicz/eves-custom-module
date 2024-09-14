#pragma once

#include <cstdint>

namespace eves
{
    // CRC-16-CCITT
    std::uint16_t calculateCrc16(const std::uint8_t *buf, std::uint8_t len);
    bool isCrcValid(const std::uint8_t *receivedBuf, std::uint8_t receivedLen, std::uint16_t targetCrc);
    bool isCrcValid(const std::uint8_t *receivedBuf, std::uint8_t receivedLen, std::uint8_t targetCrcMSB, std::uint8_t targetCrcLSB);
    bool isCrcValid(const std::uint8_t *receivedBuf, std::uint8_t receivedLen);

    // Encoding
    std::uint32_t createMsgId(std::uint8_t moduleId, std::uint8_t msgType);
    void createMsgBalancingState(std::uint8_t moduleId, std::uint32_t balanceState, std::uint32_t &msgId, std::uint8_t buf[8]);
    std::uint8_t encodeMsbCellVolt(std::uint16_t cellVolt_mV);
    std::uint8_t encodeLsbCellVolt(std::uint16_t cellVolt_mV);
    std::uint8_t encodeTemperature(std::int16_t temperature);

    // Group from 0 to 7;
    void createMsgCellVoltGroup(std::uint8_t moduleId, std::uint8_t group, std::uint16_t cell1, std::uint16_t cell2, std::uint16_t cell3, std::uint32_t &msgId, std::uint8_t buf[8]);

    // Group from 0 to 2;
    void createMsgTemperatureGroup(std::uint8_t moduleId, std::uint8_t group,
                                   std::int16_t t1, std::int16_t t2, std::int16_t t3,
                                   std::int16_t t4, std::int16_t t5, std::int16_t t6,
                                   std::uint32_t &msgId, std::uint8_t buf[8]);

    // Decoding
    std::uint8_t decodeModuleId(std::uint32_t msgId);
    std::uint8_t decodeMsgType(std::uint32_t msgId);
    std::uint16_t decodeCellVoltage_mV(std::uint8_t MSB, std::uint8_t LSB);
    std::int16_t decodeTemperature(std::uint8_t data);
}