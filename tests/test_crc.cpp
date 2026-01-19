/**
 * @file test_crc.cpp
 * @brief Unit tests for CRC16 checksum functionality
 *
 * Usage:
 *   ./test_crc
 *
 * Example: Using CRC in your application
 *   Communication::TCP client;
 *   uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
 *   // CRC is automatically handled with add_crc/has_crc parameters
 *   client.writeS(data, 4, true);  // Appends CRC
 *   client.readS(buffer, 6, true); // Verifies CRC
 */

#include "test_utils.hpp"
#include "tcp_client.hpp"
#include <cstring>

using namespace Communication;

// Test helper class to access protected CRC methods
class CRCTestClient : public TCP
{
public:
    CRCTestClient() : ESC::CLI(-1, "CRCTest"), TCP(-1) {} // -1 = silent mode

    uint16_t test_crc(uint8_t *buf, int n)
    {
        return CRC(buf, n);
    }

    bool test_check_crc(uint8_t *buf, int size)
    {
        return check_CRC(buf, size);
    }
};

// Test: CRC computation is deterministic
bool test_crc_deterministic()
{
    CRCTestClient client;

    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint16_t crc1 = client.test_crc(data, 5);
    uint16_t crc2 = client.test_crc(data, 5);

    TEST_ASSERT_EQ(crc1, crc2);
    return true;
}

// Test: CRC changes when data changes
bool test_crc_detects_changes()
{
    CRCTestClient client;

    uint8_t data1[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t data2[] = {0x01, 0x02, 0x03, 0x05}; // Last byte different

    uint16_t crc1 = client.test_crc(data1, 4);
    uint16_t crc2 = client.test_crc(data2, 4);

    TEST_ASSERT_NE(crc1, crc2);
    return true;
}

// Test: CRC verification passes for valid data
bool test_crc_verification_valid()
{
    CRCTestClient client;

    // Buffer with space for CRC
    uint8_t data[6] = {0xAB, 0xCD, 0xEF, 0x12, 0x00, 0x00};

    // Compute CRC and append it
    uint16_t crc = client.test_crc(data, 4);
    data[4] = crc & 0xFF;
    data[5] = (crc >> 8) & 0xFF;

    // Verify CRC (size includes CRC bytes)
    TEST_ASSERT(client.test_check_crc(data, 6));
    return true;
}

// Test: CRC verification fails for corrupted data
bool test_crc_verification_corrupted()
{
    CRCTestClient client;

    uint8_t data[6] = {0xAB, 0xCD, 0xEF, 0x12, 0x00, 0x00};

    // Compute and append valid CRC
    uint16_t crc = client.test_crc(data, 4);
    data[4] = crc & 0xFF;
    data[5] = (crc >> 8) & 0xFF;

    // Corrupt one byte
    data[2] = 0xFF;

    // Verification should fail
    TEST_ASSERT(!client.test_check_crc(data, 6));
    return true;
}

// Test: CRC of empty data
bool test_crc_empty_data()
{
    CRCTestClient client;

    uint8_t data[] = {};
    uint16_t crc = client.test_crc(data, 0);

    // CRC of empty data should be 0 (initial accumulator value)
    TEST_ASSERT_EQ(0, crc);
    return true;
}

// Test: CRC of single byte
bool test_crc_single_byte()
{
    CRCTestClient client;

    uint8_t data1[] = {0x00};
    uint8_t data2[] = {0xFF};

    uint16_t crc1 = client.test_crc(data1, 1);
    uint16_t crc2 = client.test_crc(data2, 1);

    // Different single bytes should produce different CRCs
    TEST_ASSERT_NE(crc1, crc2);
    return true;
}

// Test: CRC table is initialized correctly (no buffer overflow)
bool test_crc_table_no_overflow()
{
    // This test verifies the fix for the CRC table buffer overflow bug
    // If the bug exists, this might crash or produce incorrect results
    CRCTestClient client;

    // Test with various data patterns
    for(int i = 0; i < 256; i++)
    {
        uint8_t data[] = {(uint8_t)i};
        uint16_t crc = client.test_crc(data, 1);
        (void)crc; // Just ensure no crash
    }

    return true;
}

int main()
{
    Test::TestRunner runner;

    runner.add_test("CRC is deterministic", test_crc_deterministic);
    runner.add_test("CRC detects data changes", test_crc_detects_changes);
    runner.add_test("CRC verification passes for valid data", test_crc_verification_valid);
    runner.add_test("CRC verification fails for corrupted data", test_crc_verification_corrupted);
    runner.add_test("CRC of empty data", test_crc_empty_data);
    runner.add_test("CRC of single byte", test_crc_single_byte);
    runner.add_test("CRC table no overflow", test_crc_table_no_overflow);

    return runner.run();
}
