#pragma once

#include "MidiSynth.h"
#include "ADSREnvelope.h"
#include "Smoothers.h"

#include "Reverb.h"
#include "Voice.h"

using namespace iplug;

enum EModulations
{
  kModGainSmoother = 0,
  kModSustainSmoother,
  //kModLFO,
  kNumModulations,
};


template<typename T>
class MySynthDSP
{
public:
#pragma mark -
  MySynthDSP(int nVoices)
  {
    for (auto i = 0; i < nVoices; i++)
    {
      // add a voice to Zone 0.
      mSynth.AddVoice(new Voice<T>(), 0);
    }

    // some MidiSynth API examples:
    // mSynth.SetKeyToPitchFn([](int k){return (k - 69.)/24.;}); // quarter-tone scale
    // mSynth.SetNoteGlideTime(0.5); // portamento
  }

  void ProcessBlock(T** inputs, T** outputs, int nOutputs, int nFrames, double qnPos = 0., bool transportIsRunning = false, double tempo = 120.)
  {
    // clear outputs
    for(auto i = 0; i < nOutputs; i++)
    {
      memset(outputs[i], 0, nFrames * sizeof(T));
    }
    
    mParamSmoother.ProcessBlock(mParamsToSmooth, mModulations.GetList(), nFrames);
    //mLFO.ProcessBlock(mModulations.GetList()[kModLFO], nFrames, qnPos, transportIsRunning, tempo);
    mSynth.ProcessBlock(mModulations.GetList(), outputs, 0, nOutputs, nFrames);
    
    for(int s=0; s < nFrames;s++)
    {
      T smoothedGain = mModulations.GetList()[kModGainSmoother][s];
      outputs[0][s] *= smoothedGain;
      outputs[1][s] *= smoothedGain;
    }
  }

  void Reset(double sampleRate, int blockSize)
  {
    mSynth.SetSampleRateAndBlockSize(sampleRate, blockSize);
    mSynth.Reset();
    mModulationsData.Resize(blockSize * kNumModulations);
    mModulations.Empty();
    
    for(int i = 0; i < kNumModulations; i++)
    {
      mModulations.Add(mModulationsData.Get() + (blockSize * i));
    }
  }

  void ProcessMidiMsg(const IMidiMsg& msg)
  {
    mSynth.AddMidiMsgToQueue(msg);
  }

  void SetParam(int paramIdx, double value)
  {
    using EEnvStage = ADSREnvelope<sample>::EStage;
    
    switch (paramIdx) {
      case kParamOvertonesMiddle:
        mSynth.ForEachVoice([value](SynthVoice& voice) { dynamic_cast<Voice<T>&>(voice).mOSC.setOvertonesMiddleCoeff(value); });
        break;
      case kParamOvertonesHigh:
        mSynth.ForEachVoice([value](SynthVoice& voice) { dynamic_cast<Voice<T>&>(voice).mOSC.setOvertonesHighCoeff(value); });
        break;
      case kParamOvertonesSep:
        mSynth.ForEachVoice([value](SynthVoice& voice) { dynamic_cast<Voice<T>&>(voice).mOSC.changeSepHarmonics(value); });
        break;
      case kParamFlageolet:
        mSynth.ForEachVoice([value](SynthVoice& voice) { dynamic_cast<Voice<T>&>(voice).mOSC.setFlageolet(value + 1); });
		break;
      case kParamGain:
        mParamsToSmooth[kModGainSmoother] = (T) value / 100.;
        break;


      case kParamAttack:
      case kParamDecay:
      case kParamRelease:
      {
        EEnvStage stage = static_cast<EEnvStage>(EEnvStage::kAttack + (paramIdx - kParamAttack));
        mSynth.ForEachVoice([stage, value](SynthVoice& voice) {
          dynamic_cast<Voice<T>&>(voice).mAMPEnv.SetStageTime(stage, value);
        });
        break;
      }
      case kParamSustain:
        mParamsToSmooth[kModSustainSmoother] = (T)value / 100.;
        break;


      case kParamReverMix:
        mSynth.ForEachVoice([value](SynthVoice& voice) { dynamic_cast<Voice<T>&>(voice).mReverb.setGain(value); });
        break;
      case kParamReverGainRefl:
        mSynth.ForEachVoice([value](SynthVoice& voice) { dynamic_cast<Voice<T>&>(voice).mReverb.setEarlyGain(value); });
        break;
      case kParamReverDelayEarly:
        mSynth.ForEachVoice([value](SynthVoice& voice) { dynamic_cast<Voice<T>&>(voice).mReverb.setEarlyDelayCoeff(value); });
        break;
      case kParamReverDelayLate:
        mSynth.ForEachVoice([value](SynthVoice& voice) { dynamic_cast<Voice<T>&>(voice).mReverb.setLateDelayCoeff(value); });
        break;
      case kParamReverSmooth:
        mSynth.ForEachVoice([value](SynthVoice& voice) { dynamic_cast<Voice<T>&>(voice).mReverb.setLateGain(value); });
        break;
      default:
        break;        
    }
  }
  
public:
  MidiSynth mSynth { VoiceAllocator::kPolyModePoly, MidiSynth::kDefaultBlockSize };
  WDL_TypedBuf<T> mModulationsData; // Sample data for global modulations (e.g. smoothed sustain)
  WDL_PtrList<T> mModulations; // Ptrlist for global modulations
  LogParamSmooth<T, kNumModulations> mParamSmoother;
  sample mParamsToSmooth[kNumModulations];
};
