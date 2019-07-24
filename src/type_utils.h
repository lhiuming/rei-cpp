#ifndef REI_TYPE_UTILS_H
#define REI_TYPE_UTILS_H

namespace rei {

// TODO check boost::noncopyable
class NoCopy {
protected:
  NoCopy() = default;
  ~NoCopy() = default;

  // allow move
  NoCopy(NoCopy&&) = default;
  NoCopy& operator=(NoCopy&&) = default;

private:
  NoCopy(const NoCopy&) = delete;
  NoCopy& operator=(const NoCopy&) = delete;
};

} // namespace rei

#endif
