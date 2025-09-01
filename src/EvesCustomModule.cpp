#include "EvesCustomModule.h"

#include <cstdint>
#include <algorithm>

namespace eves
{
    // CRC-16-CCITT
    std::uint16_t calculateCrc16(const std::uint8_t *buf, std::uint8_t len)
    {
        std::uint16_t crc = 0xFFFF;        // Initial value
        std::uint16_t polynomial = 0x1021; // CRC-16-CCITT polynomial
        len -= 2;                          // Remove CRC16 bytes

        for (std::uint8_t i = 0; i < len; ++i)
        {
            std::uint8_t byte = buf[i];
            crc ^= (byte << 8);
            for (std::uint8_t bit = 0; bit < 8; ++bit)
            {
                if (crc & 0x8000)
                {
                    crc = (crc << 1) ^ polynomial;
                }
                else
                {
                    crc <<= 1;
                }
            }
        }

        return crc;
    }

    bool isCrcValid(const std::uint8_t *receivedBuf, std::uint8_t receivedLen, std::uint16_t targetCrc)
    {
        return calculateCrc16(receivedBuf, receivedLen) == targetCrc;
    }

    bool isCrcValid(const std::uint8_t *receivedBuf, std::uint8_t receivedLen, std::uint8_t targetCrcMSB, std::uint8_t targetCrcLSB)
    {
        return isCrcValid(receivedBuf, receivedLen, static_cast<std::uint16_t>((targetCrcMSB << 8) | targetCrcLSB));
    }

    bool isCrcValid(const std::uint8_t *receivedBuf, std::uint8_t receivedLen)
    {
        return isCrcValid(receivedBuf, receivedLen - 2, receivedBuf[receivedLen - 1 - 1], receivedBuf[receivedLen - 1]);
    }

    // Encoding
    std::uint32_t createMsgId(std::uint8_t moduleId, std::uint8_t msgType)
    {
        return 0x6000 + (moduleId << 8) + msgType;
    }

    void createMsgBalancingState(std::uint8_t moduleId, std::uint32_t balanceState, std::uint32_t &msgId, std::uint8_t buf[8])
    {
        msgId = createMsgId(moduleId + 1, 0x01);
        buf[0] = 0xFF;
        buf[1] = 0xFF;
        buf[2] = (balanceState >> 24) & 0xFF;
        buf[3] = (balanceState >> 16) & 0xFF;
        buf[4] = (balanceState >> 8) & 0xFF;
        buf[5] = balanceState & 0xFF;

        const auto crc = calculateCrc16(buf, 6);
        buf[6] = (crc & 0xFF00) >> 8;
        buf[7] = crc & 0xFF;
    }

    std::uint8_t encodeMsbCellVolt(std::uint16_t cellVolt_mV)
    {
        return cellVolt_mV & 0xFF;
    }

    std::uint8_t encodeLsbCellVolt(std::uint16_t cellVolt_mV)
    {
        return (cellVolt_mV >> 8) & 0x3F;
    }

    std::uint8_t encodeMsbCellId(std::uint16_t cellId)
    {
        return (cellId & 0xFF00) >> 8;
    }

    std::uint8_t encodeLsbCellId(std::uint16_t cellId)
    {
        return cellId & 0xFF;
    }

    std::uint8_t encodeTemperature(std::int16_t temperature)
    {
        temperature += 40;
        temperature = std::max((int16_t)0, temperature);
        temperature = std::min(temperature, (int16_t)255);
        return static_cast<std::uint8_t>(temperature);
    }

    // Group from 0 to 165;
    void createMsgCellVoltGroup(std::uint8_t moduleId, std::uint8_t group, std::uint16_t cell1, std::uint16_t cell2, std::uint16_t cell3, std::uint32_t &msgId, std::uint8_t buf[8])
    {
        std::uint8_t targetGroup = 0x02 + (group % 12);
        std::uint8_t targetFrameGroup = ((std::uint8_t)(group / 12)) << 4;

        msgId = createMsgId(moduleId + 1, targetFrameGroup + targetGroup);
        buf[0] = encodeMsbCellVolt(cell1);
        buf[1] = encodeLsbCellVolt(cell1);

        buf[2] = encodeMsbCellVolt(cell2);
        buf[3] = encodeLsbCellVolt(cell2);

        buf[4] = encodeMsbCellVolt(cell3);
        buf[5] = encodeLsbCellVolt(cell3);

        const auto crc = calculateCrc16(buf, 6);
        buf[6] = (crc & 0xFF00) >> 8;
        buf[7] = crc & 0xFF;
    }

    // Group from 0 to 2;
    void createMsgTemperatureGroup(std::uint8_t moduleId, std::uint8_t group,
                                   std::int16_t t1, std::int16_t t2, std::int16_t t3,
                                   std::int16_t t4, std::int16_t t5, std::int16_t t6,
                                   std::uint32_t &msgId, std::uint8_t buf[8])
    {
        msgId = createMsgId(moduleId + 1, 0x0F - group);
        buf[0] = encodeTemperature(t1);
        buf[1] = encodeTemperature(t2);
        buf[2] = encodeTemperature(t3);
        buf[3] = encodeTemperature(t4);
        buf[4] = encodeTemperature(t5);
        buf[5] = encodeTemperature(t6);

        const auto crc = calculateCrc16(buf, 6);
        buf[6] = (crc & 0xFF00) >> 8;
        buf[7] = crc & 0xFF;
    }

    void createMsgPackData1(std::uint16_t packVolt, std::uint16_t highCellVolt, std::uint16_t lowCellVolt, std::uint32_t &msgId, std::uint8_t buf[8])
    {
        msgId = 0x8000;
        buf[0] = (packVolt & 0xFF00) >> 8;
        buf[1] = packVolt & 0xFF;

        buf[2] = encodeMsbCellVolt(highCellVolt);
        buf[3] = encodeLsbCellVolt(highCellVolt);

        buf[4] = encodeMsbCellVolt(lowCellVolt);
        buf[5] = encodeLsbCellVolt(lowCellVolt);

        const auto crc = calculateCrc16(buf, 6);
        buf[6] = (crc & 0xFF00) >> 8;
        buf[7] = crc & 0xFF;
    }

    void createMsgPackData2(std::uint16_t avgCellVolt, std::uint16_t highCellVoltId, std::uint16_t lowCellVoltId, std::uint32_t &msgId, std::uint8_t buf[8])
    {

        msgId = 0x8010;
        buf[0] = encodeMsbCellVolt(avgCellVolt);
        buf[1] = encodeLsbCellVolt(avgCellVolt);

        buf[2] = encodeMsbCellId(highCellVoltId);
        buf[3] = encodeLsbCellId(highCellVoltId);

        buf[4] = encodeMsbCellId(lowCellVoltId);
        buf[5] = encodeLsbCellId(lowCellVoltId);

        const auto crc = calculateCrc16(buf, 6);
        buf[6] = (crc & 0xFF00) >> 8;
        buf[7] = crc & 0xFF;
    }

    void createMsgPackData3(std::int16_t highTemp, std::int16_t lowTemp, std::int16_t avgTemp, std::uint32_t &msgId, std::uint8_t buf[8])
    {
        msgId = 0x8020;
        buf[0] = encodeTemperature(highTemp);
        buf[1] = encodeTemperature(lowTemp);
        buf[2] = encodeTemperature(avgTemp);

        buf[3] = 0xFF;
        buf[4] = 0xFF;
        buf[5] = 0xFF;

        const auto crc = calculateCrc16(buf, 6);
        buf[6] = (crc & 0xFF00) >> 8;
        buf[7] = crc & 0xFF;
    }

    // Decoding
    std::uint8_t decodeModuleId(std::uint32_t msgId)
    {
        return ((msgId & 0xFF00) - 0x6000) >> 8;
    }

    std::uint8_t decodeMsgType(std::uint32_t msgId)
    {
        return msgId & 0x00FF;
    }

    std::uint16_t decodeCellVoltage_mV(std::uint8_t MSB, std::uint8_t LSB)
    {
        return MSB + (LSB & 0x3F) * 256;
    }

    std::uint16_t decodePackVoltage_dV(std::uint8_t MSB, std::uint8_t LSB)
    {
        return (MSB << 8) + LSB;
    }

    std::uint16_t decodeCellId(std::uint8_t MSB, std::uint8_t LSB)
    {
        return (MSB << 8) + LSB;
    }

    std::int16_t decodeTemperature(std::uint8_t data)
    {
        return data - 40;
    }
}