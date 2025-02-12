//--------------------------------------------------------------------------
// Copyright (C) 2020-2021 Cisco and/or its affiliates. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 2 as published
// by the Free Software Foundation.  You may not use, modify or distribute
// this program under any other version of the GNU General Public License.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//--------------------------------------------------------------------------

// dce_smb2_file.h author Dipta Pandit <dipandit@cisco.com>

#ifndef DCE_SMB2_FILE_H
#define DCE_SMB2_FILE_H

// This provides file tracker for SMBv2

#include "dce_smb2.h"

class Dce2Smb2TreeTracker;

class Dce2Smb2FileTracker
{
public:

    Dce2Smb2FileTracker() = delete;
    Dce2Smb2FileTracker(const Dce2Smb2FileTracker& arg) = delete;
    Dce2Smb2FileTracker& operator=(const Dce2Smb2FileTracker& arg) = delete;

    Dce2Smb2FileTracker(uint64_t file_idv, Dce2Smb2TreeTracker* p_tree) : ignore(true),
        file_name_len(0), file_offset(0), file_id(file_idv), file_size(0), file_name_hash(0),
        file_name(nullptr), direction(FILE_DOWNLOAD), smb2_pdu_state(DCE2_SMB_PDU_STATE__COMMAND),
        parent_tree(p_tree)
    {
        debug_logf(dce_smb_trace, GET_CURRENT_PACKET,
            "file tracker %" PRIu64 " created\n", file_id);
        memory::MemoryCap::update_allocations(sizeof(*this));
    }

    ~Dce2Smb2FileTracker();
    bool process_data(const uint8_t*, uint32_t, uint64_t);
    bool process_data(const uint8_t*, uint32_t);
    bool close();
    void set_info(char*, uint16_t, uint64_t, bool = false);
    void accept_raw_data_from(Dce2Smb2SessionData*);

    bool accepting_raw_data()
    { return (smb2_pdu_state == DCE2_SMB_PDU_STATE__RAW_DATA); }

    void set_direction(FileDirection dir) { direction = dir; }
    Dce2Smb2TreeTracker* get_parent() { return parent_tree; }
    uint64_t get_file_id() { return file_id; }

private:
    void file_detect();
    bool ignore;
    uint16_t file_name_len;
    uint64_t file_offset;
    uint64_t file_id;
    uint64_t file_size;
    uint64_t file_name_hash;
    char* file_name;
    FileDirection direction;
    Dce2SmbPduState smb2_pdu_state;
    Dce2Smb2TreeTracker* parent_tree;
};

using  Dce2Smb2FileTrackerMap =
    std::unordered_map<uint64_t, Dce2Smb2FileTracker*, std::hash<uint64_t> >;

#endif

