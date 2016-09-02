//---------------------------------------------------------- -*- Mode: C++ -*-
// $Id$
//
// Created 2016/05/11
// Author: Mike Ovsiannikov
//
// Copyright 2016 Quantcast Corp.
//
// This file is part of Kosmos File System (KFS).
//
// Licensed under the Apache License, Version 2.0
// (the "License"); you may not use this file except in compliance with
// the License. You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.
//
// Veiwstamped replications RPCs..
//
//
//----------------------------------------------------------------------------

#ifndef META_VROPS_H
#define META_VROPS_H

#include "common/kfstypes.h"
#include "common/kfsdecls.h"

#include "MetaRequest.h"
#include "LogWriter.h"
#include "MetaVrSM.h"
#include "util.h"

#include <errno.h>

namespace KFS
{
class Properties;

const char* const kMetaVrViewSeqFieldNamePtr              = "VV";
const char* const kMetaVrEpochSeqFieldNamePtr             = "VE";
const char* const kMetaVrCommittedFieldNamePtr            = "VC";
const char* const kMetaVrCommittedErrChecksumFieldNamePtr = "VCE";
const char* const kMetaVrCommittedFidSeedFieldNamePtr     = "VCF";
const char* const kMetaVrCommittedStatusFieldNamePtr      = "VCS";
const char* const kMetaVrLastLogSeqFieldNamePtr           = "VL";
const char* const kMetaVrStateFieldNamePtr                = "VS";
const char* const kMetaVrFsIdFieldNamePtr                 = "VI";
const char* const kMetaVrMDSLocationHostFieldNamePtr      = "VH";
const char* const kMetaVrMDSLocationPortFieldNamePtr      = "VP";
const char* const kMetaVrClusterKeyFieldNamePtr           = "VK";
const char* const kMetaVrMetaMdFieldNamePtr               = "VM";

class MetaVrRequest : public MetaRequest
{
public:
    typedef MetaVrSM::Config::NodeId NodeId;

    seq_t          mEpochSeq;
    seq_t          mViewSeq;
    MetaVrLogSeq   mCommittedSeq;
    int64_t        mCommittedErrChecksum;
    fid_t          mCommittedFidSeed;
    int            mCommittedStatus;
    MetaVrLogSeq   mLastLogSeq;
    NodeId         mNodeId;
    int            mCurState;
    int64_t        mFileSystemId;
    string         mMetaDataStoreHost;
    int            mMetaDataStorePort;
    string         mClusterKey;
    string         mMetaMd;

    seq_t          mRetCurEpochSeq;
    seq_t          mRetCurViewSeq;
    MetaVrLogSeq   mRetCommittedSeq;
    int64_t        mRetCommittedErrChecksum;
    fid_t          mRetCommittedFidSeed;
    int            mRetCommittedStatus;
    MetaVrLogSeq   mRetLastLogSeq;
    int            mRetCurState;
    int64_t        mRetFileSystemId;
    string         mRetClusterKey;
    string         mRetMetaMd;
    ServerLocation mRetMetaDataStoreLocation;

    MetaVrRequest(
        MetaOp    inOpType,
        LogAction inLogAction,
        seq_t     inOpSequence = -1)
        : MetaRequest(inOpType, inLogAction, inOpSequence),
          mEpochSeq(-1),
          mViewSeq(-1),
          mCommittedSeq(),
          mCommittedErrChecksum(0),
          mCommittedFidSeed(-1),
          mCommittedStatus(0),
          mLastLogSeq(),
          mNodeId(-1),
          mCurState(-1),
          mFileSystemId(-1),
          mMetaDataStoreHost(),
          mMetaDataStorePort(-1),
          mClusterKey(),
          mMetaMd(),
          mRetCurEpochSeq(-1),
          mRetCurViewSeq(-1),
          mRetCommittedSeq(),
          mRetCommittedErrChecksum(0),
          mRetCommittedFidSeed(-1),
          mRetCommittedStatus(0),
          mRetLastLogSeq(),
          mRetCurState(-1),
          mRetFileSystemId(-1),
          mRetClusterKey(),
          mRetMetaMd(),
          mRetMetaDataStoreLocation(),
          mVrSMPtr(0),
          mRefCount(0),
          mScheduleCommitFlag(false)
    {
        MetaVrRequest::Ref();
        shortRpcFormatFlag = false;
        replayBypassFlag   = true;
    }
    bool Validate() const
    {
        return (0 <= mEpochSeq && 0 <= mViewSeq);
    }
    template<typename T>
    static T& ParserDef(
        T& inParser)
    {
        return MetaRequest::ParserDef(inParser)
        .Def("E",  &MetaVrRequest::mEpochSeq,              seq_t(-1))
        .Def("V",  &MetaVrRequest::mViewSeq,               seq_t(-1))
        .Def("C",  &MetaVrRequest::mCommittedSeq                    )
        .Def("CE", &MetaVrRequest::mCommittedErrChecksum, int64_t(0))
        .Def("CF", &MetaVrRequest::mCommittedFidSeed,      seq_t(-1))
        .Def("CS", &MetaVrRequest::mCommittedStatus,               0)
        .Def("L",  &MetaVrRequest::mLastLogSeq                      )
        .Def("N",  &MetaVrRequest::mNodeId,               NodeId(-1))
        .Def("S",  &MetaVrRequest::mCurState,                     -1)
        .Def("I",  &MetaVrRequest::mFileSystemId,        int64_t(-1))
        .Def("MP", &MetaVrRequest::mMetaDataStorePort,            -1)
        .Def("MH", &MetaVrRequest::mMetaDataStoreHost               )
        .Def("CK", &MetaVrRequest::mClusterKey                      )
        .Def("MD", &MetaVrRequest::mMetaMd                          )
        ;
    }
    void Request(
        ReqOstream& inStream) const;
    virtual bool start()
    {
        if (0 == status && ! Validate()) {
            status = -EINVAL;
        }
        return (0 == status);
    }
    virtual void HandleResponse(
        seq_t                 inSeq,
        const Properties&     inProps,
        NodeId                inNodeId,
        const ServerLocation& inPeer) = 0;
    void Ref()
        { mRefCount++; }
    void Unref()
    {
        if (--mRefCount <= 0) {
            delete this;
        }
    }
    virtual void handle();
    MetaVrSM* GetVrSMPtr() const
        { return mVrSMPtr; }
    void SetVrSMPtr(
        MetaVrSM* inPtr)
        { mVrSMPtr = inPtr; }
    virtual void response(
        ReqOstream& inOs)
    {
        ResponseHeader(inOs);
        if (0 <= mRetCurViewSeq) {
            inOs << kMetaVrViewSeqFieldNamePtr <<
                ":" << mRetCurViewSeq << "\r\n";
        }
        if (0 <= mRetCurEpochSeq) {
            inOs << kMetaVrEpochSeqFieldNamePtr <<
                ":" << mRetCurEpochSeq << "\r\n";
        }
        if (0 <= mRetCurState) {
            inOs << kMetaVrStateFieldNamePtr <<
                ":" << mRetCurState << "\r\n";
        }
        if (mRetCommittedSeq.IsValid()) {
            inOs << kMetaVrCommittedFieldNamePtr <<
                ":" << mRetCommittedSeq << "\r\n";
            inOs << kMetaVrCommittedErrChecksumFieldNamePtr <<
                ":" << mCommittedErrChecksum << "\r\n";
            inOs << kMetaVrCommittedFidSeedFieldNamePtr <<
                ":" << mCommittedFidSeed << "\r\n";
            inOs << kMetaVrCommittedStatusFieldNamePtr <<
                ":" << mCommittedStatus << "\r\n";
        }
        if (mRetLastLogSeq.IsValid()) {
            inOs << kMetaVrLastLogSeqFieldNamePtr <<
                ":" << mRetLastLogSeq << "\r\n";
        }
        if (mRetLastLogSeq.IsValid()) {
            inOs << kMetaVrFsIdFieldNamePtr <<
                ":" << mRetFileSystemId << "\r\n";
        }
        if (0 < mRetMetaDataStoreLocation.port) {
            inOs << kMetaVrMDSLocationPortFieldNamePtr <<
                ":" << mRetMetaDataStoreLocation.port << "\r\n";
        }
        if (! mRetMetaDataStoreLocation.hostname.empty()) {
            inOs << kMetaVrMDSLocationHostFieldNamePtr <<
                ":" << mRetMetaDataStoreLocation.hostname << "\r\n";
        }
        if (! mRetClusterKey.empty()) {
            inOs << kMetaVrClusterKeyFieldNamePtr <<
                ":" << mRetClusterKey <<  "\r\n";
        }
        if (! mRetMetaMd.empty()) {
            inOs << kMetaVrMetaMdFieldNamePtr <<
                ":" << mRetMetaMd <<  "\r\n";
        }
        inOs << "\r\n";
    }
    void SetScheduleCommit()
        { mScheduleCommitFlag = ! replayFlag && mCommittedSeq.IsValid(); }
protected:
    MetaVrSM* mVrSMPtr;
    int       mRefCount;
    bool      mScheduleCommitFlag;

    virtual ~MetaVrRequest()
    {
        if (0 != mRefCount) {
            panic("~MetaVrRequest: invalid ref count");
        }
    }
    bool ResponseHeader(
        ReqOstream& inOs);
    template<typename T>
    void HandleReply(
        T&                    inReq,
        seq_t                 inSeq,
        const Properties&     inProps,
        NodeId                inNodeId,
        const ServerLocation& inPeer)
    {
        if (mVrSMPtr) {
            mVrSMPtr->HandleReply(inReq, inSeq, inProps, inNodeId, inPeer);
        }
    }
    virtual void ReleaseSelf()
        { Unref(); }
    ostream& ShowFields(
        ostream& inOs) const
    {
        return (inOs <<
            " epoch: "     << mEpochSeq <<
            " view: "      << mViewSeq <<
            " committed: " << mCommittedSeq <<
            " last: "      << mLastLogSeq <<
            " node: "      << mNodeId <<
            " state: "     << MetaVrSM::GetStateName(mCurState) <<
            " fsid: "      << mFileSystemId <<
            " mds: "       << mMetaDataStoreHost <<
            " "            << mMetaDataStorePort <<
            " ckey: "      << mClusterKey <<
            " md: "        << mMetaMd
        );
    }
private:
    MetaVrRequest(
        const MetaVrRequest& inRequest);
    MetaVrRequest& operator=(
        const MetaVrRequest& inRequest);
};

class MetaVrHello : public MetaVrRequest
{
public:
    MetaVrHello()
        : MetaVrRequest(META_VR_HELLO, kLogIfOk)
        {}
    virtual ostream& ShowSelf(
        ostream& inOs) const
        { return ShowFields(inOs << "vr-hello"); }
    virtual void HandleResponse(
        seq_t                 inSeq,
        const Properties&     inProps,
        NodeId                inNodeId,
        const ServerLocation& inPeer)
        { HandleReply(*this, inSeq, inProps, inNodeId, inPeer); }
protected:
    virtual ~MetaVrHello()
        {}
};

class MetaVrStartViewChange : public MetaVrRequest
{
public:
    MetaVrStartViewChange()
        : MetaVrRequest(META_VR_START_VIEW_CHANGE, kLogIfOk)
        {}
    virtual ostream& ShowSelf(
        ostream& inOs) const
        { return ShowFields(inOs << "vr-start-view-change"); }
    virtual void HandleResponse(
        seq_t                 inSeq,
        const Properties&     inProps,
        NodeId                inNodeId,
        const ServerLocation& inPeer)
        { HandleReply(*this, inSeq, inProps, inNodeId, inPeer); }
protected:
    virtual ~MetaVrStartViewChange()
        {}
};

class MetaVrDoViewChange : public MetaVrRequest
{
public:
    NodeId mPimaryNodeId;

    MetaVrDoViewChange()
        : MetaVrRequest(META_VR_DO_VIEW_CHANGE, kLogIfOk),
          mPimaryNodeId(-1)
        {}
    virtual ostream& ShowSelf(
        ostream& inOs) const
    {
        return (
            ShowFields(inOs << "vr-do-view-change") <<
            " primary: " << mPimaryNodeId
        );
    }
    virtual void HandleResponse(
        seq_t                 inSeq,
        const Properties&     inProps,
        NodeId                inNodeId,
        const ServerLocation& inPeer)
        { HandleReply(*this, inSeq, inProps, inNodeId, inPeer); }
    template<typename T>
    static T& ParserDef(
        T& inParser)
    {
        return MetaVrRequest::ParserDef(inParser)
        .Def("P", &MetaVrDoViewChange::mPimaryNodeId,  seq_t(-1))
        ;
    }
protected:
    virtual ~MetaVrDoViewChange()
        {}
};

class MetaVrStartView : public MetaVrRequest
{
public:
    MetaVrStartView()
        : MetaVrRequest(META_VR_START_VIEW, kLogIfOk)
        {}
    virtual ostream& ShowSelf(
        ostream& inOs) const
        { return ShowFields(inOs << "vr-start-view"); }
    virtual void HandleResponse(
        seq_t                 inSeq,
        const Properties&     inProps,
        NodeId                inNodeId,
        const ServerLocation& inPeer)
        { HandleReply(*this, inSeq, inProps, inNodeId, inPeer); }
protected:
    virtual ~MetaVrStartView()
        {}
};

class MetaVrReconfiguration : public MetaIdempotentRequest
{
public:
    enum
    {
        kOpTypeNone            = 0,
        kOpTypeAddNode         = 1,
        kOpTypeRemoveNodes     = 2,
        kOpTypeActivateNodes   = 3,
        kOpTypeInactivateNodes = 4,
        kOpTypeSetPrimaryOrder = 5,
        kOpTypeSetParameters   = 6,
        kOpTypesCount
    };
    typedef MetaVrSM::Config Config;
    typedef Config::NodeId   NodeId;
    typedef Config::Flags    Flags;

    int             mOpType;
    int             mListSize;
    int             mPrimaryOrder;
    Flags           mNodeFlags;
    NodeId          mNodeId;
    StringBufT<256> mListStr;

    MetaVrReconfiguration()
        : MetaIdempotentRequest(META_VR_RECONFIGURATION, kLogIfOk),
          mOpType(kOpTypeNone),
          mListSize(0),
          mPrimaryOrder(0),
          mNodeFlags(Config::kFlagsNone),
          mNodeId(-1),
          mListStr()
        {}
    virtual ostream& ShowSelf(
        ostream& inOs) const
    {
        return (inOs <<
            "vr-reconfiguration:"
            " type: "  << mOpType <<
            " flags: " << mNodeFlags <<
            " node: "  << mNodeId <<
            " list:"
            " size: "  << mListSize <<
            " "        << mListStr
        );
    }
    virtual bool start();
    bool Validate()
    {
        return ((kOpTypeAddNode == mOpType ? 0 <= mNodeId : 0 < mListSize) &&
            kOpTypeNone < mOpType && mOpType < kOpTypesCount);
    }
    virtual void handle();
    virtual void response(
        ReqOstream& inStream);
    template<typename T>
    static T& ParserDef(T& parser)
    {
        return MetaIdempotentRequest::ParserDef(parser)
        .Def2("Op-type",   "T", &MetaVrReconfiguration::mOpType,
                int(kOpTypeNone))
        .Def2("List-size", "S", &MetaVrReconfiguration::mListSize, 0)
        .Def2("Flags",     "F", &MetaVrReconfiguration::mNodeFlags,
                Flags(Config::kFlagsNone))
        .Def2("Prim-ord",  "O", &MetaVrReconfiguration::mPrimaryOrder, 0)
        .Def2("Node-id",   "N", &MetaVrReconfiguration::mNodeId,
                NodeId(-1))
        .Def2("List",      "L", &MetaVrReconfiguration::mListStr)
        ;
    }
    template<typename T>
    static T& IoParserDef(T& parser)
    {
        // Keep everything except list for debugging.
        return MetaIdempotentRequest::IoParserDef(parser)
        .Def("T", &MetaVrReconfiguration::mOpType,             int(kOpTypeNone))
        .Def("S", &MetaVrReconfiguration::mListSize,                          0)
        .Def("F", &MetaVrReconfiguration::mNodeFlags, Flags(Config::kFlagsNone))
        .Def("O", &MetaVrReconfiguration::mPrimaryOrder,                      0)
        .Def("N", &MetaVrReconfiguration::mNodeId,                   NodeId(-1))
        ;
    }
    template<typename T>
    static T& LogIoDef(T& parser)
    {
        return MetaIdempotentRequest::LogIoDef(parser)
        .Def("T", &MetaVrReconfiguration::mOpType,             int(kOpTypeNone))
        .Def("S", &MetaVrReconfiguration::mListSize,                          0)
        .Def("F", &MetaVrReconfiguration::mNodeFlags, Flags(Config::kFlagsNone))
        .Def("O", &MetaVrReconfiguration::mPrimaryOrder,                      0)
        .Def("N", &MetaVrReconfiguration::mNodeId,                   NodeId(-1))
        .Def("L", &MetaVrReconfiguration::mListStr)
        ;
    }
protected:
    virtual ~MetaVrReconfiguration()
        {}
};

class MetaVrLogStartView : public MetaRequest
{
public:
    typedef MetaVrSM::Config::NodeId NodeId;

    MetaVrLogSeq mCommittedSeq;
    MetaVrLogSeq mNewLogSeq;
    NodeId       mNodeId;
    int64_t      mTime;
    bool         mHandledFlag;

    MetaVrLogStartView()
        : MetaRequest(META_VR_LOG_START_VIEW, kLogIfOk),
          mCommittedSeq(),
          mNewLogSeq(),
          mNodeId(-1),
          mTime(0),
          mHandledFlag(false)
        {}
    bool Validate()
    {
        return (
            0 <= mNodeId &&
            mCommittedSeq.IsValid() &&
            mNewLogSeq.IsValid() &&
            0 < mNewLogSeq.mLogSeq &&
            (mCommittedSeq.mEpochSeq < mNewLogSeq.mEpochSeq ||
                mCommittedSeq.mViewSeq < mNewLogSeq.mViewSeq) &&
            (! logseq.IsValid() || (mCommittedSeq < logseq &&
                logseq < mNewLogSeq))
        );
    }
    virtual bool start()
    {
        if (! Validate()) {
            status    = -EINVAL;
            statusMsg = "invalid VR vr-log-start-view";
        }
        return (0 == status);
    }
    virtual void handle();
    virtual void response(
        ReqOstream& /* inStream */)
    {
        panic("vr-log-start-view: unexpected response method invocation");
    }
    virtual ostream& ShowSelf(
        ostream& inOs) const
    {
        return (inOs <<
            "vr-log-start-view:"
            " committed: " << mCommittedSeq <<
            " new log: "   << mNewLogSeq <<
            " primary: "   << mNodeId
        );
    }
    template<typename T>
    static T& LogIoDef(T& parser)
    {
        return MetaRequest::LogIoDef(parser)
        .Def("C",  &MetaVrLogStartView::mCommittedSeq      )
        .Def("L",  &MetaVrLogStartView::mNewLogSeq         )
        .Def("N",  &MetaVrLogStartView::mNodeId, NodeId(-1))
        .Def("T",  &MetaVrLogStartView::mTime,   int64_t(0))
        ;
    }
protected:
    virtual ~MetaVrLogStartView()
        {}
};

} // namespace KFS

#endif /* META_VROPS_H */