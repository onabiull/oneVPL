/*############################################################################
  # Copyright (C) 2005 Intel Corporation
  #
  # SPDX-License-Identifier: MIT
  ############################################################################*/

#ifndef __PIPELINE_ENCODE_BRC_H__
#define __PIPELINE_ENCODE_BRC_H__

#include "sample_defs.h"

#ifndef MFX_VERSION
    #error MFX_VERSION not defined
#endif

#include "vpl/mfxbrc.h"

#include <algorithm>

#define MFX_CHECK_NULL_PTR1(pointer) MSDK_CHECK_POINTER(pointer, MFX_ERR_NULL_PTR);

#define MFX_CHECK_NULL_PTR2(pointer1, pointer2) \
    MFX_CHECK_NULL_PTR1(pointer1);              \
    MFX_CHECK_NULL_PTR1(pointer2);

#define MFX_CHECK_NULL_PTR3(pointer1, pointer2, pointer3) \
    MFX_CHECK_NULL_PTR2(pointer1, pointer2);              \
    MFX_CHECK_NULL_PTR1(pointer3);

#define MFX_CHECK(cond, error) MSDK_CHECK_NOT_EQUAL(cond, true, error)
#define MFX_CHECK_STS(sts)     MFX_CHECK(sts == MFX_ERR_NONE, sts)

/*
NalHrdConformance | VuiNalHrdParameters   |  Result
--------------------------------------------------------------
    off                  any                => MFX_BRC_NO_HRD
    default              off                => MFX_BRC_NO_HRD
    on                   off                => MFX_BRC_HRD_WEAK
    on (or default)      on (or default)    => MFX_BRC_HRD_STRONG
--------------------------------------------------------------
*/
#if !defined(__MFXENCTOOLS_INT_H__) || (MFX_VERSION < MFX_VERSION_NEXT)
enum : mfxU16 {
    MFX_BRC_NO_HRD = 0,
    MFX_BRC_HRD_WEAK, // IF HRD CALCULATION IS REQUIRED, BUT NOT WRITTEN TO THE STREAM
    MFX_BRC_HRD_STRONG
};
#endif

class cBRCParams {
public:
    mfxU16 rateControlMethod; // CBR or VBR

    mfxU16 HRDConformance; // is HRD compliance  needed
    mfxU16 bRec; // is Recoding possible
    mfxU16 bPanic; // is Panic mode possible

    // HRD params
    mfxU32 bufferSizeInBytes;
    mfxU32 initialDelayInBytes;

    // Sliding window parameters
    mfxU16 WinBRCMaxAvgKbps;
    mfxU16 WinBRCSize;

    // RC params
    mfxU32 targetbps;
    mfxU32 maxbps;
    mfxF64 frameRate;
    mfxF64 inputBitsPerFrame;
    mfxF64 maxInputBitsPerFrame;
    mfxU32 maxFrameSizeInBits;

    // Frame size params
    mfxU16 width;
    mfxU16 height;
    mfxU16 chromaFormat;
    mfxU16 bitDepthLuma;
    mfxU32 mRawFrameSizeInBits;
    mfxU32 mRawFrameSizeInPixs;

    // GOP params
    mfxU16 gopPicSize;
    mfxU16 gopRefDist;
    bool bPyr;
    bool bFieldMode;

    //BRC accurancy params
    mfxF64 fAbPeriodLong; // number on frames to calculate abberation from target frame
    mfxF64 fAbPeriodShort; // number on frames to calculate abberation from target frame
    mfxF64 dqAbPeriod; // number on frames to calculate abberation from dequant
    mfxF64 bAbPeriod; // number of frames to calculate abberation from target bitrate

    //QP parameters
    mfxI32 quantOffset;
    mfxI32 quantMaxI;
    mfxI32 quantMinI;
    mfxI32 quantMaxP;
    mfxI32 quantMinP;
    mfxI32 quantMaxB;
    mfxI32 quantMinB;
    bool mMBBRC;
    mfxU32 codecId;

public:
    cBRCParams()
            : rateControlMethod(0),
              HRDConformance(MFX_BRC_NO_HRD),
              bRec(0),
              bPanic(0),
              bufferSizeInBytes(0),
              initialDelayInBytes(0),
              WinBRCMaxAvgKbps(0),
              WinBRCSize(0),
              targetbps(0),
              maxbps(0),
              frameRate(0),
              inputBitsPerFrame(0),
              maxInputBitsPerFrame(0),
              maxFrameSizeInBits(0),
              width(0),
              height(0),
              chromaFormat(0),
              bitDepthLuma(0),
              mRawFrameSizeInBits(0),
              mRawFrameSizeInPixs(0),
              gopPicSize(0),
              gopRefDist(0),
              bPyr(0),
              bFieldMode(0),
              fAbPeriodLong(0),
              fAbPeriodShort(0),
              dqAbPeriod(0),
              bAbPeriod(0),
              quantOffset(0),
              quantMaxI(0),
              quantMinI(0),
              quantMaxP(0),
              quantMinP(0),
              quantMaxB(0),
              quantMinB(0),
              mMBBRC(false),
              codecId(0) {}

    mfxStatus Init(mfxVideoParam* par, bool bFieldMode = false);
    mfxStatus GetBRCResetType(mfxVideoParam* par,
                              bool bNewSequence,
                              bool& bReset,
                              bool& bSlidingWindowReset);
};

struct BRC_Ctx {
    mfxI32 QuantI; //currect qp for intra frames
    mfxI32 QuantP; //currect qp for P frames
    mfxI32 QuantB; //currect qp for B frames

    mfxI32 Quant; // qp for last encoded frame
    mfxI32 QuantMin; // qp Min for last encoded frame (is used for recoding)
    mfxI32 QuantMax; // qp Max for last encoded frame (is used for recoding)

    bool bToRecode; // last frame is needed in recoding
    bool bPanic; // last frame is needed in panic
    mfxU32 encOrder; // encoding order of last encoded frame
    mfxU32 poc; // poc of last encoded frame
    mfxI32 SceneChange; // scene change parameter of last encoded frame
    mfxU32 SChPoc; // poc of frame with scene change
    mfxU32 LastIEncOrder; // encoded order if last intra frame
    mfxU32 LastNonBFrameSize; // encoded frame size of last non B frame (is used for sceneChange)

    mfxF64 fAbLong; // avarage frame size (long period)
    mfxF64 fAbShort; // avarage frame size (short period)
    mfxF64 dQuantAb; // avarage dequant
    mfxF64 totalDeviation; // deviation from  target bitrate (total)

    mfxF64
        eRate; // eRate of last encoded frame, this parameter is used for scene change calculation
    mfxF64
        eRateSH; // eRate of last encoded scene change frame, this parameter is used for scene change calculation
};
class AVGBitrate {
public:
    AVGBitrate(mfxU32 windowSize, mfxU32 maxBitPerFrame, mfxU32 avgBitPerFrame)
            : m_maxWinBits(maxBitPerFrame * windowSize),
              m_maxWinBitsLim(0),
              m_avgBitPerFrame(std::min(avgBitPerFrame, maxBitPerFrame)),
              m_currPosInWindow(windowSize - 1),
              m_lastFrameOrder(mfxU32(-1))

    {
        windowSize = windowSize > 0 ? windowSize : 1; // kw
        m_slidingWindow.resize(windowSize);
        for (mfxU32 i = 0; i < windowSize; i++) {
            m_slidingWindow[i] = maxBitPerFrame / 3; //initial value to prevent big first frames
        }
        m_maxWinBitsLim = GetMaxWinBitsLim();
    }
    virtual ~AVGBitrate() {}
    void UpdateSlidingWindow(mfxU32 sizeInBits,
                             mfxU32 FrameOrder,
                             bool bPanic,
                             bool bSH,
                             mfxU32 recode) {
        mfxU32 windowSize = (mfxU32)m_slidingWindow.size();
        bool bNextFrame   = FrameOrder != m_lastFrameOrder;

        if (bNextFrame) {
            m_lastFrameOrder  = FrameOrder;
            m_currPosInWindow = (m_currPosInWindow + 1) % windowSize;
        }
        m_slidingWindow[m_currPosInWindow] = sizeInBits;

        if (bNextFrame) {
            if (bPanic || bSH) {
                m_maxWinBitsLim =
                    mfx::clamp((GetLastFrameBits(windowSize, false) + m_maxWinBits) / 2,
                               GetMaxWinBitsLim(),
                               m_maxWinBits);
            }
            else {
                if (recode) {
                    m_maxWinBitsLim =
                        mfx::clamp(GetLastFrameBits(windowSize, false) + GetStep() / 2,
                                   m_maxWinBitsLim,
                                   m_maxWinBits);
                }
                else if ((m_maxWinBitsLim > GetMaxWinBitsLim() + GetStep()) &&
                         (m_maxWinBitsLim - GetStep() >
                          (GetLastFrameBits(windowSize - 1, false) + sizeInBits)))
                    m_maxWinBitsLim -= GetStep();
            }
        }
    }
    mfxU32 GetMaxFrameSize(bool bPanic, bool bSH, mfxU32 recode) {
        mfxU32 winBits = GetLastFrameBits(GetWindowSize() - 1, !bPanic);

        mfxU32 maxWinBitsLim = m_maxWinBitsLim;
        if (bSH)
            maxWinBitsLim = (m_maxWinBits + m_maxWinBitsLim) / 2;
        if (bPanic)
            maxWinBitsLim = m_maxWinBits;
        maxWinBitsLim = std::min(maxWinBitsLim + recode * GetStep() / 2, m_maxWinBits);

        mfxU32 maxFrameSize = winBits >= m_maxWinBitsLim
                                  ? mfxU32(std::max(mfxI32(m_maxWinBits - winBits), 1))
                                  : maxWinBitsLim - winBits;

        return maxFrameSize;
    }
    mfxU32 GetWindowSize() {
        return (mfxU32)m_slidingWindow.size();
    }

protected:
    mfxU32 m_maxWinBits;
    mfxU32 m_maxWinBitsLim;
    mfxU32 m_avgBitPerFrame;

    mfxU32 m_currPosInWindow;
    mfxU32 m_lastFrameOrder;
    std::vector<mfxU32> m_slidingWindow;

    mfxU32 GetLastFrameBits(mfxU32 numFrames, bool bCheckSkip) {
        mfxU32 size = 0;
        numFrames = numFrames < m_slidingWindow.size() ? numFrames : (mfxU32)m_slidingWindow.size();
        for (mfxU32 i = 0; i < numFrames; i++) {
            mfxU32 frame_size = m_slidingWindow[(m_currPosInWindow + m_slidingWindow.size() - i) %
                                                m_slidingWindow.size()];
            if (bCheckSkip && (frame_size < m_avgBitPerFrame / 3))
                frame_size = m_avgBitPerFrame / 3;
            size += frame_size;
            //printf("GetLastFrames: %d) %d sum %d\n",i,m_slidingWindow[(m_currPosInWindow + m_slidingWindow.size() - i) % m_slidingWindow.size() ], size);
        }
        return size;
    }
    mfxU32 GetStep() {
        return (m_maxWinBits / GetWindowSize() - m_avgBitPerFrame) / 2;
    }

    mfxU32 GetMaxWinBitsLim() {
        return m_maxWinBits - GetStep() * GetWindowSize();
    }
};
struct sHrdInput {
    bool m_cbrFlag               = false;
    mfxU32 m_bitrate             = 0;
    mfxU32 m_maxCpbRemovalDelay  = 0;
    mfxF64 m_clockTick           = 0.0;
    mfxF64 m_cpbSize90k          = 0.0;
    mfxF64 m_initCpbRemovalDelay = 0;

    void Init(cBRCParams par);
};
class HRDCodecSpec {
private:
    mfxI32 m_overflowQuant  = 999;
    mfxI32 m_underflowQuant = 0;

public:
    mfxI32 GetMaxQuant() const {
        return m_overflowQuant - 1;
    }
    mfxI32 GetMinQuant() const {
        return m_underflowQuant + 1;
    }
    void SetOverflowQuant(mfxI32 qp) {
        m_overflowQuant = qp;
    }
    void SetUndeflowQuant(mfxI32 qp) {
        m_underflowQuant = qp;
    }
    void ResetQuant() {
        m_overflowQuant  = 999;
        m_underflowQuant = 0;
    }

public:
    virtual ~HRDCodecSpec() {}
    virtual void Init(cBRCParams& par)                               = 0;
    virtual void Reset(cBRCParams& par)                              = 0;
    virtual void Update(mfxU32 sizeInbits, mfxU32 eo, bool bSEI)     = 0;
    virtual mfxU32 GetInitCpbRemovalDelay(mfxU32 eo) const           = 0;
    virtual mfxU32 GetInitCpbRemovalDelayOffset(mfxU32 eo) const     = 0;
    virtual mfxU32 GetMaxFrameSizeInBits(mfxU32 eo, bool bSEI) const = 0;
    virtual mfxU32 GetMinFrameSizeInBits(mfxU32 eo, bool bSEI) const = 0;
    virtual mfxF64 GetBufferDeviation(mfxU32 eo) const               = 0;
    virtual mfxF64 GetBufferDeviationFactor(mfxU32 eo) const         = 0;
};

class HEVC_HRD : public HRDCodecSpec {
public:
    HEVC_HRD()
            : m_prevAuCpbRemovalDelayMinus1(0),
              m_prevAuCpbRemovalDelayMsb(0),
              m_prevAuFinalArrivalTime(0),
              m_prevBpAuNominalRemovalTime(0),
              m_prevBpEncOrder(0) {}
    virtual ~HEVC_HRD() {}
    void Init(cBRCParams& par) override;
    void Reset(cBRCParams& par) override;
    void Update(mfxU32 sizeInbits, mfxU32 eo, bool bSEI) override;
    mfxU32 GetInitCpbRemovalDelay(mfxU32 eo) const override;
    mfxU32 GetInitCpbRemovalDelayOffset(mfxU32 eo) const override {
        return mfxU32(m_hrdInput.m_cpbSize90k - GetInitCpbRemovalDelay(eo));
    }
    mfxU32 GetMaxFrameSizeInBits(mfxU32 eo, bool bSEI) const override;
    mfxU32 GetMinFrameSizeInBits(mfxU32 eo, bool bSEI) const override;
    mfxF64 GetBufferDeviation(mfxU32 eo) const override;
    mfxF64 GetBufferDeviationFactor(mfxU32 eo) const override;

protected:
    sHrdInput m_hrdInput;
    mfxI32 m_prevAuCpbRemovalDelayMinus1;
    mfxU32 m_prevAuCpbRemovalDelayMsb;
    mfxF64 m_prevAuFinalArrivalTime;
    mfxF64 m_prevBpAuNominalRemovalTime;
    mfxU32 m_prevBpEncOrder;
};

class H264_HRD : public HRDCodecSpec {
public:
    H264_HRD();
    virtual ~H264_HRD() {}
    void Init(cBRCParams& par) override;
    void Reset(cBRCParams& par) override;
    void Update(mfxU32 sizeInbits, mfxU32 eo, bool bSEI) override;
    mfxU32 GetInitCpbRemovalDelay(mfxU32 eo) const override;
    mfxU32 GetInitCpbRemovalDelayOffset(mfxU32 eo) const override;
    mfxU32 GetMaxFrameSizeInBits(mfxU32 eo, bool bSEI) const override;
    mfxU32 GetMinFrameSizeInBits(mfxU32 eo, bool bSEI) const override;
    mfxF64 GetBufferDeviation(mfxU32 eo) const override;
    mfxF64 GetBufferDeviationFactor(mfxU32 eo) const override;

private:
    sHrdInput m_hrdInput;
    double m_trn_cur; // nominal removal time
    double m_taf_prv; // final arrival time of prev unit
};

class ExtBRC {
private:
    cBRCParams m_par;
    std::unique_ptr<HRDCodecSpec> m_hrdSpec;
    bool m_bInit;
    BRC_Ctx m_ctx;
    std::unique_ptr<AVGBitrate> m_avg;
    std::vector<mfxExtMBQP> m_MBQP;
    std::vector<mfxU8> m_MBQPBuff;
    std::vector<mfxExtBuffer*> m_ExtBuff;

public:
    ExtBRC()
            : m_par(),
              m_hrdSpec(),
              m_bInit(false),
              m_ctx({}),
              m_avg(),
              m_MBQP(),
              m_MBQPBuff(),
              m_ExtBuff() {}
    mfxStatus Init(mfxVideoParam* par);
    mfxStatus Reset(mfxVideoParam* par);
    mfxStatus Close() {
        m_bInit = false;
        return MFX_ERR_NONE;
    }
    mfxStatus GetFrameCtrl(mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl);
    mfxStatus Update(mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl, mfxBRCFrameStatus* status);

protected:
    mfxI32 GetCurQP(mfxU32 type, mfxI32 layer);
};

namespace HEVCExtBRC {
inline mfxStatus Init(mfxHDL pthis, mfxVideoParam* par) {
    MFX_CHECK_NULL_PTR1(pthis);
    return ((ExtBRC*)pthis)->Init(par);
}
inline mfxStatus Reset(mfxHDL pthis, mfxVideoParam* par) {
    MFX_CHECK_NULL_PTR1(pthis);
    return ((ExtBRC*)pthis)->Reset(par);
}
inline mfxStatus Close(mfxHDL pthis) {
    MFX_CHECK_NULL_PTR1(pthis);
    return ((ExtBRC*)pthis)->Close();
}

inline mfxStatus GetFrameCtrl(mfxHDL pthis, mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl) {
    MFX_CHECK_NULL_PTR1(pthis);
    return ((ExtBRC*)pthis)->GetFrameCtrl(par, ctrl);
}
inline mfxStatus Update(mfxHDL pthis,
                        mfxBRCFrameParam* par,
                        mfxBRCFrameCtrl* ctrl,
                        mfxBRCFrameStatus* status) {
    MFX_CHECK_NULL_PTR1(pthis);
    return ((ExtBRC*)pthis)->Update(par, ctrl, status);
}

inline mfxStatus Create(mfxExtBRC& m_BRC) {
    MFX_CHECK(m_BRC.pthis == NULL, MFX_ERR_UNDEFINED_BEHAVIOR);
    m_BRC.pthis        = new ExtBRC;
    m_BRC.Init         = Init;
    m_BRC.Reset        = Reset;
    m_BRC.Close        = Close;
    m_BRC.GetFrameCtrl = GetFrameCtrl;
    m_BRC.Update       = Update;
    return MFX_ERR_NONE;
}
inline mfxStatus Destroy(mfxExtBRC& m_BRC) {
    if (m_BRC.pthis != NULL) {
        delete (ExtBRC*)m_BRC.pthis;
        m_BRC.pthis        = 0;
        m_BRC.Init         = 0;
        m_BRC.Reset        = 0;
        m_BRC.Close        = 0;
        m_BRC.GetFrameCtrl = 0;
        m_BRC.Update       = 0;
    }
    return MFX_ERR_NONE;
}
} // namespace HEVCExtBRC
#endif
