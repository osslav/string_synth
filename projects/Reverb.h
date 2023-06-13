#pragma once


#include <queue>
#include <atomic>
template <typename T>
class Reverb
{
private:
  const double sample_rate_ = 44100;
  // std::atomic<int> diff = 0;

  double delay_ = 5000.0;
  double mix_ = 1.0;
  double gain_ = 1.0;

  // std::queue<T> old_vals_;
  const int size_buf_ = 44100;
  T buffer_[44100];
  int index_next_val = 0;
  int index_old_val = 0 + delay_;

public:
  Reverb()
  {
    for (int i = 0; i < size_buf_; ++i)
      buffer_[i] = 0.0;
  }

  /*
T Process(T curr_val)
{
  if (delay_ != 0)
  {
      if (diff.load() == 0)
    {
      T old_val = old_vals_.front();
      old_vals_.push((curr_val + old_val) * mix_);
      old_vals_.pop();
      return old_val * gain_;
    }

      if (diff.load() < 0)
    {
      ++diff;
      old_vals_.pop();
      return 0;
    }

      if (diff.load() > 0)
    {
      --diff;
      old_vals_.push((curr_val) * mix_);
      return 0;
    }

      return 0;
  }
  else
    return 0;
}
  */

  T Process(T curr_val)
  {
    T next_val = buffer_[index_next_val];
    buffer_[index_old_val] = (curr_val + next_val) * mix_;

    ++index_old_val;
    if (index_old_val >= size_buf_)
      index_old_val = 0;

    ++index_next_val;
    if (index_next_val >= size_buf_)
      index_next_val = 0;

    return next_val * gain_;
  }

  void setParam(int paramIdx, double val)
  {
    if (paramIdx == kParamReverMix)
      setMix(val);

    if (paramIdx == kParamReverGain)
      setGain(val);

    if (paramIdx == kParamReverDelay)
      setDelay(val);
  }

  void setDelay(double new_delay_ms)
  {
    double new_delay = new_delay_ms * sample_rate_ / 1000;
    // diff.store(new_delay - delay_);
    /*
    if (diff > 0)
      for (int i = 0; i < diff; ++i)
        old_vals_.push(0);
    else
      for (int i = 0; i > diff; --i)
        old_vals_.pop();
        */

    delay_ = new_delay;
  }

  void setGain(double new_gain) { gain_ = new_gain / 100; }

  void setMix(double new_mix) { mix_ = new_mix / 100.0; }
};