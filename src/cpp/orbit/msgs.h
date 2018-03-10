#pragma once

// k273 includes
#include <k273/util.h>

///////////////////////////////////////////////////////////////////////////////

namespace K273::Orbit::Connection {

    const uint32_t MsgType_Hello = 1;
    const uint32_t MsgType_InitialiseClient = 2;
    const uint32_t MsgType_Ping = 3;
    const uint32_t MsgType_Pong = 4;

    // each message sent is this
    struct MessageHeader {
        uint32_t message_type;

        // inclusive of header
        uint32_t message_length;

    } PACKED;

    // message sent on connect from client -> server
    struct HelloMessage {
        MessageHeader header;

        // if -1, server will assign an id
        int source_id;

    } PACKED;

    // submessage, from server -> client
    struct ChannelInfo {
        uint32_t channel_id;

        // 0 - dedicated inbound
        // 1 - dedicated outbound
        // 2 - master inbound
        // 3 - master outbound
        uint16_t inbound_outbound;

        // outbound message group - if 'dedicated outbound', otherwise ignored
        uint16_t outbound_channel_group_id;

        // number of cache lines 1024
        uint32_t size_in_cache_lines;

        // needs to be long enough to include for path
        char path[128];

    } PACKED;

    // initialise the client, server -> client
    struct InitialiseClientMessage {
        MessageHeader header;
        char orbit_name[32];

        uint32_t poll_type;

        uint32_t number_of_channels;
        ChannelInfo channel_info[0];

    } PACKED;

    // keepalive, from server -> client
    struct PingMessage {
        MessageHeader header;
    } PACKED;

    // keepalive response, from client -> server
    struct PongMessage {
        MessageHeader header;
    } PACKED;


    struct BundleHeader {
        uint32_t bundle_length;

        uint16_t message_count;
        uint16_t channel_id;

        uint16_t orbit_source_id;
        uint16_t orbit_target_id;

        uint32_t orbit_sequence_id;

        // to keep integrity on a single channel
        uint32_t skip_sequence_count;

        uint32_t originate_epoch;
        uint32_t originate_nanonsecs;

        uint8_t data[0];

    } PACKED;

}
