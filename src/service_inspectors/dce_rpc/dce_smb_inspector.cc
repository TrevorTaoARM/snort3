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

// dce_smb_inspector.h author Dipta Pandit <dipandit@cisco.com>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dce_smb_inspector.h"

#include "dce_smb_common.h"
#include "dce_smb_module.h"
#include "dce_smb_utils.h"
#include "dce_smb2_session_cache.h"

using namespace snort;

Dce2Smb::Dce2Smb(const dce2SmbProtoConf& pc)
{
    config = pc;
}

Dce2Smb::~Dce2Smb()
{
    if (config.smb_invalid_shares)
    {
        DCE2_ListDestroy(config.smb_invalid_shares);
    }
}

void Dce2Smb::show(const SnortConfig*) const
{
    print_dce2_smb_conf(config);
}

void Dce2Smb::eval(Packet* p)
{
    Profile profile(dce2_smb_pstat_main);

    assert(p->has_tcp_data());
    assert(p->flow);

    Dce2SmbFlowData* smb_flowdata =
        (Dce2SmbFlowData*)p->flow->get_flow_data(Dce2SmbFlowData::inspector_id);

    Dce2SmbSessionData* smb_session_data;
    if (smb_flowdata)
        smb_session_data = smb_flowdata->get_smb_session_data();
    else
        smb_session_data = create_new_smb_session(p, &config);

    if (smb_session_data)
    {
        dce2_detected = 0;
        p->packet_flags |= PKT_ALLOW_MULTIPLE_DETECT;
        p->endianness = new DceEndianness();

        smb_session_data->process();

        //smb_session_data may not be valid anymore in case of upgrade
        //but flow will always have updated session
        if (!dce2_detected)
            DCE2_Detect(get_dce2_session_data(p->flow));

        delete(p->endianness);
        p->endianness = nullptr;
    }
    else
    {
        debug_logf(dce_smb_trace, p, "non-smb packet detected\n");
    }
}

void Dce2Smb::clear(Packet* p)
{
    DCE2_SsnData* sd = get_dce2_session_data(p->flow);
    if (sd)
        DCE2_ResetRopts(sd, p);
}

//-------------------------------------------------------------------------
// api stuff
//-------------------------------------------------------------------------

size_t session_cache_size;

static Module* mod_ctor()
{
    return new Dce2SmbModule;
}

static void mod_dtor(Module* m)
{
    delete m;
}

static void dce2_smb_init()
{
    Dce2SmbFlowData::init();
    DCE2_SmbInitGlobals();
    DCE2_SmbInitDeletePdu();
    DceContextData::init(DCE2_TRANS_TYPE__SMB);
}

static void dce2_smb_thread_int()
{
    DCE2_SmbSessionCacheInit(session_cache_size);
}

static void dce_smb_thread_term()
{
    delete smb2_session_cache;
}

static size_t get_max_smb_session(dce2SmbProtoConf* config)
{
    size_t smb_sess_storage_req = (sizeof(Dce2Smb2SessionTracker) +
        sizeof(Dce2Smb2TreeTracker) + sizeof(Dce2Smb2RequestTracker) +
        (sizeof(Dce2Smb2FileTracker) * SMB_AVG_FILES_PER_SESSION));

    size_t max_smb_mem = DCE2_ScSmbMemcap(config);

    return (max_smb_mem/smb_sess_storage_req);
}

static Inspector* dce2_smb_ctor(Module* m)
{
    Dce2SmbModule* mod = (Dce2SmbModule*)m;
    dce2SmbProtoConf config;
    mod->get_data(config);
    session_cache_size = get_max_smb_session(&config);
    return new Dce2Smb(config);
}

static void dce2_smb_dtor(Inspector* p)
{
    delete p;
}

const InspectApi dce2_smb_api =
{
    {
        PT_INSPECTOR,
        sizeof(InspectApi),
        INSAPI_VERSION,
        0,
        API_RESERVED,
        API_OPTIONS,
        DCE2_SMB_NAME,
        DCE2_SMB_HELP,
        mod_ctor,
        mod_dtor
    },
    IT_SERVICE,
    PROTO_BIT__PDU,
    nullptr,  // buffers
    "netbios-ssn",
    dce2_smb_init,
    nullptr, // pterm
    dce2_smb_thread_int, // tinit
    dce_smb_thread_term, // tterm
    dce2_smb_ctor,
    dce2_smb_dtor,
    nullptr, // ssn
    nullptr  // reset
};

