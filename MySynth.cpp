#include "MySynth.h"
#include "IPlug_include_in_plug_src.h"
#include "LFO.h"

#include "IPlugPaths.h"
#include "IconsForkAwesome.h"
#include "IconsFontaudio.h"

MySynth::MySynth(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  //create control
  GetParam(kParamGain)->InitDouble("Level", 70., 0., 100.0, 0.1, "%");
  GetParam(kParamOvertonesMiddle)->InitDouble("Overtones middle", 0.6, 0., 1., 0.01);
  GetParam(kParamOvertonesHigh)->InitDouble("Overtones high", 0.1, 0., 1., 0.01);
  GetParam(kParamOvertonesSep)->InitDouble("middle/high", 0.2, 0., 1., 0.01);
  GetParam(kParamFlageolet)->InitEnum("", 0, 4, "", IParam::kFlagsNone, "", "off", "1/2", "1/3", "1/4");

  GetParam(kParamAttack)->InitDouble("Attack", 10., 1., 1000., 0.1, "ms", IParam::kFlagsNone, "ADSR", IParam::ShapePowCurve(3.));
  GetParam(kParamDecay)->InitDouble("Decay", 5., 5., 1000., 0.1, "ms", IParam::kFlagsNone, "ADSR", IParam::ShapePowCurve(3.));
  GetParam(kParamSustain)->InitDouble("Sustain", 50., 0., 100., 1, "%", IParam::kFlagsNone, "ADSR");
  GetParam(kParamRelease)->InitDouble("Release", 3000., 2., 10000., 0.1, "ms", IParam::kFlagsNone, "ADSR");
  
  GetParam(kParamReverMix)->InitDouble("Mix", 0.8, 0., 1.0, 0.01);
  GetParam(kParamReverGainRefl)->InitDouble("Gain", 0.8, 0., 1.0, 0.01);
  GetParam(kParamReverSmooth)->InitDouble("Smooth", 0.6, 0., 1.0, 0.01);
  GetParam(kParamReverDelayEarly)->InitDouble("Delay early", 0.15, 0., 1.0, 0.02);
  GetParam(kParamReverDelayLate)->InitDouble("Delay late", 0.15, 0., 1.0, 0.02);

    
#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]()
  {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics)
  {
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);

    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    //pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->AttachBackground(PNGBACKGROUND_FN);
    pGraphics->EnableMouseOver(true);
    pGraphics->EnableMultiTouch(true);
    
#ifdef OS_WEB
    pGraphics->AttachPopupMenuControl();
#endif

//    pGraphics->EnableLiveEdit(true);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IBitmap knobRotateBitmap = pGraphics->LoadBitmap(PNGKNOBROTATE_FN, 1, false, 2);
    const IBitmap sliderHandleBitmap = pGraphics->LoadBitmap(PNGSLIDERHANDLE_FN);
    const IBitmap sliderTrackBitmap = pGraphics->LoadBitmap(PNGSLIDERTRACK_FN);
    const IBitmap sliderTrackLongBitmap = pGraphics->LoadBitmap(PNGSLIDERTRACKLONG_FN);
    const IBitmap sliderHandleLongBitmap = pGraphics->LoadBitmap(PNGSLIDERHANDLELONG_FN);

    //base rect
    const IRECT b = pGraphics->GetBounds().GetPadded(-20.f);    

    //keyboards rect
    IRECT keyboardBounds = b.GetFromBottom(300);
    IRECT wheelsBounds = keyboardBounds.ReduceFromLeft(100.f).GetPadded(-10.f);
    pGraphics->AttachControl(new IVKeyboardControl(keyboardBounds), kCtrlTagKeyboard);
    pGraphics->AttachControl(new IWheelControl(wheelsBounds.FracRectHorizontal(1.0)), kCtrlTagBender);
    //pGraphics->AttachControl(new IWheelControl(wheelsBounds.FracRectHorizontal(0.5, true), IMidiMsg::EControlChangeMsg::kModWheel));
//    pGraphics->AttachControl(new IVMultiSliderControl<4>(b.GetGridCell(0, 2, 2).GetPadded(-30), "", DEFAULT_STYLE, kParamAttack, EDirection::Vertical, 0.f, 1.f));

    //control rect(gain and etc)
    const IRECT controls = b.GetGridCell(0, 2, 1);
    const IRECT main_controls = controls.GetGridCell(0, 1, 3);
    //pGraphics->AttachControl(new ITextControl(main_controls.GetGridCell(0, 5, 1), "Obertones", IText(40)));
    pGraphics->AttachControl(new IBKnobRotaterControl(main_controls.GetGridCell(2, 5, 2).GetCentredInside(90), knobRotateBitmap, kParamOvertonesMiddle));
    pGraphics->AttachControl(new IBKnobRotaterControl(main_controls.GetGridCell(3, 5, 2).GetCentredInside(90), knobRotateBitmap, kParamOvertonesHigh));
    //pGraphics->AttachControl(new IVSliderControl(main_controls.GetGridCell(4, 5, 1), kParamOvertonesSep, "Middle/High", DEFAULT_STYLE, false, iplug::igraphics::EDirection::Horizontal));
    pGraphics->AttachControl(new IBSliderControl(main_controls.GetGridCell(3, 5, 1).GetTranslated(0, -10).GetCentredInside((float)sliderTrackLongBitmap.W(), (float)sliderTrackLongBitmap.H()), sliderHandleLongBitmap, sliderTrackLongBitmap, kParamOvertonesSep, EDirection::Horizontal));
    pGraphics->AttachControl(new IVSlideSwitchControl(main_controls.GetGridCell(21, 8, 3).GetTranslated(20, 0), kParamFlageolet, "", DEFAULT_STYLE, true));

	//reverb rect
    const IRECT reverb = controls.GetGridCell(1, 1, 3);

	pGraphics->AttachControl(new IBKnobRotaterControl(reverb.GetGridCell(3, 4, 5).GetTranslated(0, 20).GetCentredInside(90), knobRotateBitmap, kParamGain));

    pGraphics->AttachControl(new IBKnobRotaterControl(reverb.GetGridCell(6, 4, 5).GetTranslated(0, -30).GetCentredInside(90), knobRotateBitmap, kParamReverMix));
	//pGraphics->AttachControl(new IVKnobControl(reverb.GetCentredInside(100).GetVShifted(-150).GetHShifted(75), kMaster));				//make on/off later

	//pGraphics->AttachControl(new ITextControl(reverb.GetGridCell(0, 5, 1), "Reverb", IText(40)));

    pGraphics->AttachControl(new IBKnobRotaterControl(reverb.GetGridCell(11, 4, 5).GetTranslated(0, -15).GetCentredInside(90), knobRotateBitmap, kParamReverDelayEarly));
    pGraphics->AttachControl(new IBKnobRotaterControl(reverb.GetGridCell(13, 4, 5).GetTranslated(0, -15).GetCentredInside(90), knobRotateBitmap, kParamReverSmooth));

    pGraphics->AttachControl(new IBKnobRotaterControl(reverb.GetGridCell(16, 4, 5).GetTranslated(0, 0).GetCentredInside(90), knobRotateBitmap, kParamReverDelayLate));
    pGraphics->AttachControl(new IBKnobRotaterControl(reverb.GetGridCell(18, 4, 5).GetTranslated(0, 0).GetCentredInside(90), knobRotateBitmap, kParamReverGainRefl));

    /*
	pGraphics->AttachControl(new ITextControl(reverb.GetVShifted(-50).GetMidVPadded(50), "Reverb Control", IText(50)));
    pGraphics->AttachControl(new IVKnobControl(reverb.GetCentredInside(100).GetVShifted(-150).GetHShifted(-75), kParamReverMix));
	//pGraphics->AttachControl(new IVKnobControl(reverb.GetCentredInside(100).GetVShifted(-150).GetHShifted(75), kMaster));				//make on/off later

    pGraphics->AttachControl(new IVKnobControl(reverb.GetCentredInside(100).GetVShifted(50).GetHShifted(-75), kParamReverGainRefl));
	pGraphics->AttachControl(new IVKnobControl(reverb.GetCentredInside(100).GetVShifted(50).GetHShifted(75), kParamReverSmooth));

    pGraphics->AttachControl(new IVKnobControl(reverb.GetCentredInside(100).GetVShifted(150).GetHShifted(-75), kParamReverDelayEarly));
	pGraphics->AttachControl(new IVKnobControl(reverb.GetCentredInside(100).GetVShifted(150).GetHShifted(75), kParamReverDelayLate));
	*/

    //sliders rect(adsr and vol meter)
    const IRECT sliders = controls.GetGridCell(8, 1, 12).Union(controls.GetGridCell(9, 1, 12)).Union(controls.GetGridCell(10, 1, 12)).GetTranslated(0, -10);
    //pGraphics->AttachControl(new IVSliderControl(sliders.GetGridCell(0, 1, 4).GetMidHPadded(30.), kParamAttack, "Attack"));
    pGraphics->AttachControl(new IBSliderControl(sliders.GetGridCell(0, 1, 4).GetMidHPadded(30.).GetCentredInside((float)sliderTrackBitmap.W(), (float)sliderTrackBitmap.H()), sliderHandleBitmap, sliderTrackBitmap, kParamAttack, EDirection::Vertical));
	//pGraphics->AttachControl(new IVSliderControl(sliders.GetGridCell(1, 1, 4).GetMidHPadded(30.), kParamDecay, "Decay"));
	pGraphics->AttachControl(new IBSliderControl(sliders.GetGridCell(1, 1, 4).GetMidHPadded(30.).GetCentredInside((float)sliderTrackBitmap.W(), (float)sliderTrackBitmap.H()), sliderHandleBitmap, sliderTrackBitmap, kParamDecay, EDirection::Vertical));
    //pGraphics->AttachControl(new IVSliderControl(sliders.GetGridCell(2, 1, 4).GetMidHPadded(30.), kParamSustain, "Sustain"));
	pGraphics->AttachControl(new IBSliderControl(sliders.GetGridCell(2, 1, 4).GetMidHPadded(30.).GetCentredInside((float)sliderTrackBitmap.W(), (float)sliderTrackBitmap.H()), sliderHandleBitmap, sliderTrackBitmap, kParamSustain, EDirection::Vertical));
    //pGraphics->AttachControl(new IVSliderControl(sliders.GetGridCell(3, 1, 4).GetMidHPadded(30.), kParamRelease, "Release"));
	pGraphics->AttachControl(new IBSliderControl(sliders.GetGridCell(3, 1, 4).GetMidHPadded(30.).GetCentredInside((float)sliderTrackBitmap.W(), (float)sliderTrackBitmap.H()), sliderHandleBitmap, sliderTrackBitmap, kParamRelease, EDirection::Vertical));
    

    pGraphics->AttachControl(new IVLEDMeterControl<2>(controls.GetFromRight(100).GetPadded(-30).GetTranslated(20, 0)), kCtrlTagMeter);
   
    
    pGraphics->AttachControl(new IVButtonControl(keyboardBounds.GetFromTRHC(200, 30).GetTranslated(0, -40), SplashClickActionFunc,
      "Show/Hide Keyboard", DEFAULT_STYLE.WithColor(kFG, COLOR_WHITE).WithLabelText({15.f, EVAlign::Middle})))->SetAnimationEndActionFunction(
      [pGraphics](IControl* pCaller) {
        static bool hide = false;
        pGraphics->GetControlWithTag(kCtrlTagKeyboard)->Hide(hide = !hide);
        pGraphics->Resize(PLUG_WIDTH, hide ? (PLUG_HEIGHT / 2) + 10 : PLUG_HEIGHT, pGraphics->GetDrawScale());
    });

    //#ifdef OS_IOS
//    if(!IsOOPAuv3AppExtension())
//    {
//      pGraphics->AttachControl(new IVButtonControl(b.GetFromTRHC(100, 100), [pGraphics](IControl* pCaller) {
//                               dynamic_cast<IGraphicsIOS*>(pGraphics)->LaunchBluetoothMidiDialog(pCaller->GetRECT().L, pCaller->GetRECT().MH());
//                               SplashClickActionFunc(pCaller);
//                             }, "BTMIDI"));
//    }
//#endif
    
    pGraphics->SetQwertyMidiKeyHandlerFunc([pGraphics](const IMidiMsg& msg)
      {
       pGraphics->GetControlWithTag(kCtrlTagKeyboard)->As<IVKeyboardControl>()->SetNoteFromMidi(msg.NoteNumber(), msg.StatusMsg() == IMidiMsg::kNoteOn);
      });
  };
#endif
}

#if IPLUG_DSP
void MySynth::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mDSP.ProcessBlock(nullptr, outputs, 2, nFrames, mTimeInfo.mPPQPos, mTimeInfo.mTransportIsRunning);
  mMeterSender.ProcessBlock(outputs, nFrames, kCtrlTagMeter);
  //mLFOVisSender.PushData({kCtrlTagLFOVis, {float(mDSP.mLFO.GetLastOutput())}});
}

void MySynth::OnIdle()
{
  mMeterSender.TransmitData(*this);
  //mLFOVisSender.TransmitData(*this);
}

void MySynth::OnReset()
{
  mDSP.Reset(GetSampleRate(), GetBlockSize());
  mMeterSender.Reset(GetSampleRate());
}

void MySynth::ProcessMidiMsg(const IMidiMsg& msg)
{
  TRACE;
  
  int status = msg.StatusMsg();
  
  switch (status)
  {
    case IMidiMsg::kNoteOn:
    case IMidiMsg::kNoteOff:
    case IMidiMsg::kPolyAftertouch:
    case IMidiMsg::kControlChange:
    case IMidiMsg::kProgramChange:
    case IMidiMsg::kChannelAftertouch:
    case IMidiMsg::kPitchWheel:
    {
      goto handle;
    }
    default:
      return;
  }
  
handle:
  mDSP.ProcessMidiMsg(msg);
  SendMidiMsg(msg);
}

void MySynth::OnParamChange(int paramIdx)
{
  mDSP.SetParam(paramIdx, GetParam(paramIdx)->Value());
}

bool MySynth::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  if(ctrlTag == kCtrlTagBender && msgTag == IWheelControl::kMessageTagSetPitchBendRange)
  {
    const int bendRange = *static_cast<const int*>(pData);
    mDSP.mSynth.SetPitchBendRange(bendRange);
  }
  
  return false;
}
#endif
