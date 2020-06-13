/**
 * CAN MCP2515_nb
 * Copyright 2020 WitchCraftWorks Team, All Rights Reserved
 *
 * Licensed under Apache 2.0
 */

#include "CANPacket.h"

static inline __attribute__((always_inline))
unsigned char hammingDistance(int x, int y) {
    unsigned char distance = 0;
    int xorXY = x ^ y;

    for (unsigned int i = 0; i < (8 * sizeof(int)); i++) {
        if (xorXY & (1 << i))
            distance++;
    }

    return distance;
}

static inline __attribute__((always_inline))
int determineReturnCodeByPacketStatus(CANPacket* packet) {
    if (packet->getStatus() & CANPacket::STATUS_TX_SENT) {
        return MCP2515_ERRORCODES::OK;
    } else if (packet->getStatus() & CANPacket::STATUS_TX_ABORTED) {
        if (packet->getStatus() & CANPacket::STATUS_TX_ABORT_REQUESTED) {
            return MCP2515_ERRORCODES::OK;
        }

        return MCP2515_ERRORCODES::INTR;
    } else if (packet->getStatus() & CANPacket::STATUS_TX_ERROR) {
        return MCP2515_ERRORCODES::BADF;
    }

    return MCP2515_ERRORCODES::INVAL;
}
