#ifndef UINT_H_
#define UINT_H_

#include <algorithm>
#include <array>
#include <cinttypes>
#include <iomanip>
#include <iostream>
#include <random>
#include <type_traits>


// Internal RNG
namespace {
  std::mt19937_64 rng64(14);
  uint64_t rng_leftover;
  uint64_t rng_bits_left = 0;
}

// Forward dec
template<int w_>
class SInt;

template<int w_,
         typename word_t = typename std::conditional<(w_ <= 8),
                                                     uint8_t, uint64_t>::type,
         int n_ = (w_ <= 8) ? 1 : (w_ + 64 - 1) / 64>
class UInt {
private:
  constexpr static int cmin(int wa, int wb) { return wa < wb ? wa : wb; }
  constexpr static int cmax(int wa, int wb) { return wa > wb ? wa : wb; }

public:
  UInt() {
    static_assert(w_ >= 0, "UInt bit width must be non-negative");
    for (int i=0; i < n_; i++)
      words_[i] = 0;
  }

  UInt(uint64_t initial) : UInt() {
    words_[0] = initial;
    mask_top_unused();
  }

  UInt(std::string initial) {
    if (initial.substr(0,2) != "0x") {
      std::cout << "ERROR: UInt string literal must start with 0x!" << std::endl;
      std::exit(-17);
    }
    initial.erase(0,2);
    // FUTURE: check that literal isn't too big
    int input_bits = 4*initial.length();
    int last_start = initial.length();
    for (int word=0; word < n_; word++) {
      if (word * kWordSize >= input_bits)
        words_[word] = 0;
      else {
        int word_start = std::max(static_cast<int>(initial.length())
                                  - 16*(word+1), 0);
        int word_len = std::min(16, last_start - word_start);
        last_start = word_start;
        const std::string substr = initial.substr(word_start, word_len);
        words_[word] = static_cast<uint64_t>(std::stoul(substr, nullptr, 16));
      }
    }
  }

  // NOTE: reads words right to left so literal appears to be concatted
  UInt(std::array<word_t, n_> raw_input_reversed) {
    for (int i = 0; i < n_; i++)
      words_[i] = raw_input_reversed[n_ - i - 1];
    mask_top_unused();
  }

  template<int other_w>
  explicit UInt(const UInt<other_w> &other) {
    static_assert(other_w <= w_, "Can't copy construct from wider UInt");
    for (int word=0; word < n_; word++) {
      if (word < UInt<other_w>::NW)
        words_[word] = other.words_[word];
      else
        words_[word] = 0;
    }
  }

  void rand_init() {
    for (int word=0; word < n_; word++) {
      words_[word] = 0;
    }
    // core_rand_init();
    // mask_top_unused();
  }

  template<int out_w>
  UInt<cmax(w_,out_w)> pad() const {
    return UInt<cmax(w_,out_w)>(*this);
  }

  template<int other_w>
  UInt<w_ + other_w> cat(const UInt<other_w> &other) const {
    UInt<w_ + other_w> to_return(other);
    const int offset = other_w % kWordSize;
    for (int i = 0; i < n_; i++) {
      to_return.words_[word_index(other_w) + i] |= static_cast<uint64_t>(words_[i]) <<
                                                     cap(offset);
      if ((offset != 0) && (i + 1 < to_return.NW - word_index(other_w)))
        to_return.words_[word_index(other_w) + i + 1] |= static_cast<uint64_t>(words_[i]) >>
                                                           cap(kWordSize - offset);
    }
    return to_return;
  }

  template<int other_w>
  UInt<cmax(w_, other_w) + 1> operator+(const UInt<other_w> &other) const {
    const int result_w = cmax(w_, other_w) + 1;
    UInt<result_w> self_padded = pad<result_w>();
    UInt<result_w> other_padded = other.template pad<result_w>();
    return self_padded.addw(other_padded);
  }

  UInt<w_> addw(const UInt<w_> &other) const {
    UInt<w_> result = core_add_sub<w_, false>(other);
    result.mask_top_unused();
    return result;
  }

  UInt<w_> subw(const UInt<w_> &other) const {
    UInt<w_> result(core_add_sub<w_, true>(other));
    result.mask_top_unused();
    return result;
  }

  SInt<w_ + 1> operator-() const {
    return SInt<w_+1>(0).subw(SInt<w_+1>(pad<w_+1>()));
  }

  template<int other_w>
  UInt<cmax(w_, other_w) + 1> operator-(const UInt<other_w> &other) const {
    const int result_w = cmax(w_, other_w) + 1;
    UInt<result_w> self_padded = pad<result_w>();
    UInt<result_w> other_padded = other.template pad<result_w>();
    return self_padded.subw(other_padded);
  }

  template<int other_w>
  UInt<w_ + other_w> operator*(const UInt<other_w> &other) const {
    UInt<w_ + other_w> result(0);
    uint64_t carry = 0;
    for (int i=0; i < n_; i++) {
      carry = 0;
      for (int j=0; j < other.NW; j++) {
        uint64_t prod_ll = lower(words_[i]) * lower(other.words_[j]);
        uint64_t prod_lu = lower(words_[i]) * upper(other.words_[j]);
        uint64_t prod_ul = upper(words_[i]) * lower(other.words_[j]);
        uint64_t prod_uu = upper(words_[i]) * upper(other.words_[j]);
        uint64_t lower_sum = lower(result.words_[i+j]) + lower(carry) +
                             lower(prod_ll);
        uint64_t upper_sum = upper(result.words_[i+j]) + upper(carry) +
                             upper(prod_ll) + upper(lower_sum) +
                             lower(prod_lu) + lower(prod_ul);
        result.words_[i+j] = (upper_sum << 32) | lower(lower_sum);
        carry = upper(upper_sum) + upper(prod_lu) + upper(prod_ul) + prod_uu;
      }
      if ((i+other.NW) < result.NW)
        result.words_[i + other.NW] += carry;
    }
    return result;
  }

  // this / other
  template<int other_w>
  UInt<w_> operator/(const UInt<other_w> &other) const {
    static_assert(w_ <= kWordSize, "Div not supported beyond 64b");
    static_assert(other_w <= kWordSize, "Div not supported beyond 64b");
    return UInt<w_>(as_single_word() / other.as_single_word());
  }

  // this % other
  template<int other_w>
  UInt<cmin(w_, other_w)> operator%(const UInt<other_w> &other) const {
    static_assert(w_ <= kWordSize, "Mod not supported beyond 64b");
    static_assert(other_w <= kWordSize, "Mod not supported beyond 64b");
    return UInt<cmin(w_, other_w)>(as_single_word() % other.as_single_word());
  }

  UInt<w_> operator~() const {
    UInt<w_> result;
    for (int i = 0; i < n_; i++) {
      result.words_[i] = ~words_[i];
    }
    result.mask_top_unused();
    return result;
  }

  UInt<w_> operator&(const UInt<w_> &other) const {
    UInt<w_> result;
    for (int i = 0; i < n_; i++) {
      result.words_[i] = words_[i] & other.words_[i];
    }
    return result;
  }

  UInt<w_> operator|(const UInt<w_> &other) const {
    UInt<w_> result;
    for (int i = 0; i < n_; i++) {
      result.words_[i] = words_[i] | other.words_[i];
    }
    return result;
  }

  UInt<w_> operator^(const UInt<w_> &other) const {
    UInt<w_> result;
    for (int i = 0; i < n_; i++) {
      result.words_[i] = words_[i] ^ other.words_[i];
    }
    return result;
  }

  UInt<1> andr() const {
    return *this == ~UInt<w_>(0);
  }

  UInt<1> orr() const {
    return *this != UInt<w_>(0);
  }

  UInt<1> xorr() const {
    word_t result = 0;
    for (int i = 0; i < n_; i++) {
      word_t word_parity_scratch = words_[i] ^ (words_[i] >> 1);
      word_parity_scratch ^= (word_parity_scratch >> 2);
      word_parity_scratch ^= (word_parity_scratch >> 4);
      if (WW > 8) {
        word_parity_scratch ^= (word_parity_scratch >> 8);
        word_parity_scratch ^= (word_parity_scratch >> 16);
        word_parity_scratch ^= (word_parity_scratch >> 32);
      }
      result ^= word_parity_scratch;
    }
    return UInt<1>(result & 1);
  }

  template<int hi, int lo>
  UInt<hi - lo + 1> bits() const {
    static_assert(hi >= lo, "Bits hi must be >= lo");
    static_assert(hi < w_, "Bits hi must be < width");
    static_assert(hi >= 0, "Bits hi must be non-negative");
    static_assert(lo >= 0, "Bits lo must be non-negative");
    return core_bits<hi,lo>();
  }

  template<int n>
  UInt<n> head() const {
    static_assert(n <= w_, "Head n must be <= width");
    static_assert(n >= 0, "Head n must be non-negative");
    return core_bits<w_-1, w_-n>();
  }

  template<int n>
  UInt<w_ - n> tail() const {
    static_assert(n <= w_, "Tail n must be <= width");
    static_assert(n >= 0, "Tail n must be non-negative");
    return core_bits<w_-n-1, 0>();
  }

  template<int shamt>
  UInt<w_ + shamt> shl() const {
    static_assert(shamt >= 0, "Shl shamt must be non-negative");
    return cat(UInt<shamt>(0));
  }

  template<int shamt>
  UInt<w_> shlw() const {
    return shl<shamt>().template tail<shamt>();
  }

  template<int shamt, bool shiftToZero>
  UInt<cmax(w_ - shamt,1)> shr_helper();

  template<int shamt>
  // called if shamt >= w_, so returning 1-bit wide 0
  UInt<cmax(w_ - shamt,1)> shr_helper(std::true_type) const {
    return UInt<cmax(w_ - shamt,1)>(0);
  }

  template<int shamt>
  // called if shamt > w_ (otherwise), so using bits
  UInt<cmax(w_ - shamt,1)> shr_helper(std::false_type) const {
    return bits<w_-1, shamt>();
  }

  template<int shamt>
  UInt<cmax(w_ - shamt,1)> shr() const {
    static_assert(shamt >= 0, "shift amount for shift right must be non-negative");
    // TODO: uses tag dispatch, so replace with constexpr if when we shift to C++17
    // https://stackoverflow.com/questions/43587405/constexpr-if-alternative
    return shr_helper<shamt>(std::integral_constant<bool, shamt >= w_>{});
  }

  template<int other_w>
  UInt<w_> operator>>(const UInt<other_w> &other) const {
    UInt<w_> result(0);
    uint64_t dshamt = other.as_single_word();
    uint64_t word_down = word_index(dshamt);
    uint64_t bits_down = dshamt % kWordSize;
    for (uint64_t i=word_down; i < n_; i++) {
      result.words_[i - word_down] = words_[i] >> bits_down;
      if ((bits_down != 0) && (i < n_-1))
        result.words_[i - word_down] |= words_[i + 1] << cap(kWordSize - bits_down);
    }
    return result;
  }

  template<int other_w>
  UInt<w_ + (1<<other_w) - 1> operator<<(const UInt<other_w> &other) const {
    static_assert(other_w <= kWordSize, "shift amount argument too wide");
    UInt<w_ + (1<<other_w) - 1> result(0);
    uint64_t dshamt = other.as_single_word();
    uint64_t word_up = word_index(dshamt);
    uint64_t bits_up = dshamt % kWordSize;
    for (uint64_t i=0; i < n_; i++) {
      result.words_[i + word_up] |= static_cast<uint64_t>(words_[i]) << bits_up;
      if ((bits_up != 0) && (dshamt + w_ > kWordSize) && (i + word_up + 1 < result.NW))
        result.words_[i + word_up + 1] = words_[i] >> cap(kWordSize - bits_up);
    }
    return result;
  }

  template<int other_w>
  UInt<w_> dshlw(const UInt<other_w> &other) const {
    // return operator<<(other).template bits<w_-1,0>();
    UInt<w_> result(0);
    uint64_t dshamt = other.as_single_word();
    uint64_t word_up = word_index(dshamt);
    uint64_t bits_up = dshamt % kWordSize;
    for (uint64_t i=0; i + word_up < n_; i++) {
      result.words_[i + word_up] |= words_[i] << bits_up;
      if ((bits_up != 0) && (w_ > kWordSize) && (i + word_up + 1 < n_))
        result.words_[i + word_up + 1] = words_[i] >> cap(kWordSize - bits_up);
    }
    result.mask_top_unused();
    return result;
  }

  UInt<1> operator<=(const UInt<w_> &other) const {
    for (int i=n_-1; i >= 0; i--) {
      if (words_[i] < other.words_[i]) return UInt<1>(1);
      if (words_[i] > other.words_[i]) return UInt<1>(0);
    }
    return UInt<1>(1);
  }

  UInt<1> operator>=(const UInt<w_> &other) const {
    for (int i=n_-1; i >= 0; i--) {
      if (words_[i] > other.words_[i]) return UInt<1>(1);
      if (words_[i] < other.words_[i]) return UInt<1>(0);
    }
    return UInt<1>(1);
  }

  UInt<1> operator<(const UInt<w_> &other) const {
    return ~(*this >= other);
  }

  UInt<1> operator>(const UInt<w_> &other) const {
    return ~(*this <= other);
  }

  UInt<1> operator==(const UInt<w_> &other) const {
    for (int i = 0; i < n_; i++) {
      if (words_[i] != other.words_[i])
        return UInt<1>(0);
    }
    return UInt<1>(1);
  }

  UInt<1> operator!=(const UInt<w_> &other) const {
    return ~(*this == other);
  }

  operator bool() const {
    static_assert(w_ == 1, "conversion to bool only allowed for width 1");
    return static_cast<bool>(words_[0]);
  }

  UInt<w_> asUInt() const {
    return UInt<w_>(*this);
  }

  SInt<w_> asSInt() const {
    SInt<w_> result(*this);
    result.sign_extend();
    return result;
  }

  SInt<w_ + 1> cvt() const {
    return pad<w_+1>().asSInt();
  }

  // Direct access for ops that only need small signals
  uint64_t as_single_word() const {
    static_assert(w_ <= kWordSize, "UInt too big for single uint64_t");
    return words_[0];
  }


  __attribute__((noinline))
  std::string to_bin_str() const {
    std::string str;
    str.reserve(w_);
    for (int i = n_-1; i >=0; i--) {
      int start_point = i==n_-1 ? bits_in_top_word_-1 : kWordSize-1;
      for (int j = start_point; j >= 0; j--)
        str += words_[i] & (1l << j) ? '1' : '0';
    }
    return str;
  }

  __attribute__((noinline))
  char* write_char_buf(char* buffer) const {                                  
      int idx = 0;
      for (int i = n_-1; i >= 0; i--) {
          int start_point = i == n_-1 ? bits_in_top_word_-1 : kWordSize-1;
          for (int j = start_point; j >= 0; j--) {
              buffer[idx++] = (words_[i] & (1l << j)) ? '1' : '0';
          }   
      }   

      buffer[w_] = '\0';
      return buffer;
  }


protected:
  template<int other_w>
  friend class uint_wrapper_t;

  void raw_copy_in(uint64_t *src) {
    for (int word=0; word < n_; word++)
      words_[word] = *src++;
  }

  void raw_copy_out(uint64_t *dst) {
    for (int word=0; word < n_; word++)
      *dst++ = words_[word];
  }

private:
  // Internal state
  std::array<word_t, n_> words_;

  // Access array word type
  typedef word_t WT;
  // Access array length
  const static int NW = n_;
  // Access array word type bit width
  const static int WW = std::is_same<word_t,uint64_t>::value ? 64 : 8;

  const static int bits_in_top_word_ = w_ % WW == 0 ? WW : w_ % WW;

  // Friend Access
  template<int other_w, typename other_word_t, int other_n>
  friend class UInt;

  template<int other_w>
  friend class SInt;

  template<int w>
  friend std::ostream& operator<<(std::ostream& os, const UInt<w>& ui);

  // Bit Addressing
  const static int kWordSize = 64;

  int static word_index(int bit_index) { return bit_index / kWordSize; }

  uint64_t static upper(uint64_t i) { return i >> 32; }
  uint64_t static lower(uint64_t i) { return i & 0x00000000ffffffff; }

  // Hack to prevent compiler warnings for shift amount being too large
  int static cap(int s) { return s % kWordSize; }

  // Clean up high bits
  void mask_top_unused() {
    if (bits_in_top_word_ != WW) {
      words_[n_-1] = words_[n_-1] & ((1l << cap(bits_in_top_word_)) - 1l);
    }
  }

  // Reused math operators
  template<int out_w, bool subtract>
  UInt<out_w> core_add_sub(const UInt<w_> &other) const {
    UInt<out_w> result;
    uint64_t carry = subtract;
    for (int i = 0; i < n_; i++) {
      uint64_t operand = subtract ? ~other.words_[i] : other.words_[i];
      result.words_[i] = words_[i] + operand + carry;
      carry = (result.words_[i] < words_[i]) || (result.words_[i] < operand);
    }
    // NOTE: trusts caller to set result words beyond input
    return result;
  }

  __attribute__((noinline))
  void core_rand_init() {
    // trusting mask_top_unused() will be called afterwards
    if (w_ < 64) {
      if (w_ > rng_bits_left) {
        rng_leftover = rng64();
        rng_bits_left = 64;
      }
      words_[0] = rng_leftover;
      rng_leftover = rng_leftover >> cap(w_);
      rng_bits_left -= w_;
    } else {
      for (int word=0; word < n_; word++) {
        words_[word] = rng64();
      }
    }
  }

  template<int hi, int lo>
  UInt<hi - lo + 1> core_bits() const {
    UInt<hi - lo + 1> result;
    if ((hi - lo + 1) == 0)
      return result;
    int word_down = word_index(lo);
    int bits_down = lo % kWordSize;
    for (int i=0; i < result.NW; i++) {
      result.words_[i] = words_[i + word_down] >> bits_down;
      if ((bits_down != 0) && (i + word_down + 1 < n_))
        result.words_[i] |= words_[i + word_down + 1] << cap(kWordSize - bits_down);
    }
    result.mask_top_unused();
    return result;
  }


  void print_to_stream(std::ostream& os) const {
    os << "0x" << std::hex << std::setfill('0');
    int top_nibble_width = (bits_in_top_word_ + 3) / 4;
    os << std::setw(top_nibble_width);
    uint64_t top_word_mask = bits_in_top_word_ == kWordSize ? -1 :
                               (1l << cap(bits_in_top_word_)) - 1;
    os << (static_cast<uint64_t>(words_[n_-1]) & top_word_mask);
    for (int word=n_ - 2; word >= 0; word--) {
     os << std::hex << std::setfill('0') << std::setw(16) << words_[word];
    }
    os << std::dec;
  }
};



template<int w>
std::ostream& operator<<(std::ostream& os, const UInt<w>& ui) {
  ui.print_to_stream(os);
  os << "<U" << w << ">";
  return os;
}

#endif  // UINT_H_
