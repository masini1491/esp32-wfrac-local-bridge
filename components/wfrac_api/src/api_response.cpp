#include "wfrac_api/api_response.hpp"

#include <cstring>
#include <limits>

#include "wfrac_protocol/base64.hpp"

namespace wfrac_api { namespace {
constexpr int kMaxDepth = 16;
struct Span { const uint8_t* begin = nullptr; const uint8_t* end = nullptr; };
class Reader {
 public:
  Reader(wfrac_protocol::ByteView in) : p_(in.data), end_(in.data == nullptr ? nullptr : in.data + in.size) {}
  ApiResponseError ws() { while (p_ < end_ && (*p_ == ' ' || *p_ == '\n' || *p_ == '\r' || *p_ == '\t')) ++p_; return ApiResponseError::none; }
  bool at(uint8_t c) { ws(); return p_ < end_ && *p_ == c; }
  bool take(uint8_t c) { if (!at(c)) return false; ++p_; return true; }
  const uint8_t* pos() const { return p_; }
  bool done() { ws(); return p_ == end_; }
  ApiResponseError string(Span* out) {
    ws(); if (p_ >= end_ || *p_++ != '"') return ApiResponseError::invalid_field_type; const uint8_t* begin=p_;
    while (p_ < end_) { uint8_t c=*p_++; if (c=='"') { *out={begin,p_-1}; return ApiResponseError::none; } if (c < 0x20) return ApiResponseError::malformed_json; if (c!='\\') continue; if(p_>=end_) return ApiResponseError::malformed_json; c=*p_++; if (c=='u') { int hi=hex4(); if(hi<0) return ApiResponseError::malformed_json; if(hi>=0xD800&&hi<=0xDBFF){ if(p_+2>end_||p_[0]!='\\'||p_[1]!='u') return ApiResponseError::malformed_json; p_+=2; int lo=hex4(); if(lo<0xDC00||lo>0xDFFF) return ApiResponseError::malformed_json; } else if(hi>=0xDC00&&hi<=0xDFFF) return ApiResponseError::malformed_json; } else if (!(c=='"'||c=='\\'||c=='/'||c=='b'||c=='f'||c=='n'||c=='r'||c=='t')) return ApiResponseError::malformed_json; }
    return ApiResponseError::malformed_json;
  }
  ApiResponseError integer(int64_t* value) {
    ws(); if(p_>=end_) return ApiResponseError::invalid_field_type; bool neg=false; if(*p_=='-'){neg=true;++p_;} if(p_>=end_||*p_<'0'||*p_>'9') return ApiResponseError::invalid_field_type; if(*p_=='0'&&p_+1<end_&&p_[1]>='0'&&p_[1]<='9') return ApiResponseError::malformed_json; uint64_t n=0; while(p_<end_&&*p_>='0'&&*p_<='9'){uint8_t d=*p_++-'0'; if(n>(std::numeric_limits<uint64_t>::max()-d)/10) return ApiResponseError::integer_overflow; n=n*10+d;} if(p_<end_&&(*p_=='.'||*p_=='e'||*p_=='E')) return ApiResponseError::invalid_field_type; const uint64_t lim=neg?uint64_t(std::numeric_limits<int64_t>::max())+1U:uint64_t(std::numeric_limits<int64_t>::max()); if(n>lim) return ApiResponseError::integer_overflow; *value=neg?(n==lim?std::numeric_limits<int64_t>::min():-static_cast<int64_t>(n)):static_cast<int64_t>(n); return ApiResponseError::none;
  }
  ApiResponseError value(int depth) {
    ws(); if(depth>kMaxDepth||p_>=end_) return depth>kMaxDepth?ApiResponseError::nesting_too_deep:ApiResponseError::malformed_json; if(*p_=='"'){Span s;return string(&s);} if(*p_=='{') return object_skip(depth+1); if(*p_=='[') return array_skip(depth+1); if(*p_=='-'||(*p_>='0'&&*p_<='9')){int64_t n;return integer(&n);} if(match("true")||match("false")||match("null")) return ApiResponseError::none; return ApiResponseError::malformed_json;
  }
 private:
  int hex4(){if(p_+4>end_)return -1;int v=0;for(int i=0;i<4;++i){uint8_t c=*p_++;int d=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;if(d<0)return -1;v=(v<<4)|d;}return v;}
  bool match(const char* s){const uint8_t* q=p_;while(*s){if(q>=end_||*q++!=static_cast<uint8_t>(*s++))return false;}p_=q;return true;}
  ApiResponseError object_skip(int depth){if(!take('{'))return ApiResponseError::malformed_json;if(take('}'))return ApiResponseError::none;for(;;){Span k;auto e=string(&k);if(e!=ApiResponseError::none)return e;if(!take(':'))return ApiResponseError::malformed_json;e=value(depth);if(e!=ApiResponseError::none)return e;if(take('}'))return ApiResponseError::none;if(!take(','))return ApiResponseError::malformed_json;}}
  ApiResponseError array_skip(int depth){if(!take('['))return ApiResponseError::malformed_json;if(take(']'))return ApiResponseError::none;for(;;){auto e=value(depth);if(e!=ApiResponseError::none)return e;if(take(']'))return ApiResponseError::none;if(!take(','))return ApiResponseError::malformed_json;}}
  const uint8_t* p_; const uint8_t* end_;
};
bool eq(Span s,const char* text){size_t n=std::strlen(text);return static_cast<size_t>(s.end-s.begin)==n&&std::memcmp(s.begin,text,n)==0;}
ApiResponseError decode(Span s,wfrac_protocol::MutableByteView out,std::size_t* size){if(out.size&&out.data)out.data[0]=0;size_t w=0;for(auto p=s.begin;p<s.end;){uint32_t cp=*p++;if(cp=='\\'){if(p>=s.end)return ApiResponseError::malformed_json;uint8_t c=*p++;if(c=='u'){auto h=[&](uint8_t x)->int{return x>='0'&&x<='9'?x-'0':x>='a'&&x<='f'?x-'a'+10:x>='A'&&x<='F'?x-'A'+10:-1;};if(p+4>s.end)return ApiResponseError::malformed_json;cp=0;for(int i=0;i<4;++i){int d=h(*p++);if(d<0)return ApiResponseError::malformed_json;cp=(cp<<4)|d;}if(cp>=0xD800&&cp<=0xDBFF){if(p+6>s.end||p[0]!='\\'||p[1]!='u')return ApiResponseError::malformed_json;p+=2;uint32_t lo=0;for(int i=0;i<4;++i){int d=h(*p++);if(d<0)return ApiResponseError::malformed_json;lo=(lo<<4)|d;}if(lo<0xDC00||lo>0xDFFF)return ApiResponseError::malformed_json;cp=0x10000+((cp-0xD800)<<10)+(lo-0xDC00);}else if(cp>=0xDC00&&cp<=0xDFFF)return ApiResponseError::malformed_json;}else{switch(c){case '"':cp='"';break;case '\\':cp='\\';break;case '/':cp='/';break;case 'b':cp='\b';break;case 'f':cp='\f';break;case 'n':cp='\n';break;case 'r':cp='\r';break;case 't':cp='\t';break;default:return ApiResponseError::malformed_json;}}}uint8_t b[4];size_t n=cp<0x80?1:cp<0x800?2:cp<0x10000?3:4;if(n==1)b[0]=cp;else if(n==2){b[0]=0xC0|(cp>>6);b[1]=0x80|(cp&63);}else if(n==3){b[0]=0xE0|(cp>>12);b[1]=0x80|((cp>>6)&63);b[2]=0x80|(cp&63);}else{b[0]=0xF0|(cp>>18);b[1]=0x80|((cp>>12)&63);b[2]=0x80|((cp>>6)&63);b[3]=0x80|(cp&63);}if(w+n>=out.size||out.data==nullptr)return ApiResponseError::output_too_small;std::memcpy(out.data+w,b,n);w+=n;}out.data[w]=0;*size=w;return w?ApiResponseError::none:ApiResponseError::missing_required_field;}
}
ApiResponseError parse_api_response_envelope(wfrac_protocol::ByteView input,const char* expected,ApiResponseEnvelopeView* out){if(out)*out={};if(!out||!expected||!*expected||(input.size&& !input.data))return ApiResponseError::invalid_argument;Reader r(input);if(!r.take('{'))return ApiResponseError::malformed_json;bool cmd=false,res=false,contents=false;Span cv{},content{};if(!r.take('}'))for(;;){Span k;auto e=r.string(&k);if(e!=ApiResponseError::none)return e;if(!r.take(':'))return ApiResponseError::malformed_json;if(eq(k,"command")){if(cmd)return ApiResponseError::malformed_json;e=r.string(&cv);if(e!=ApiResponseError::none)return e;cmd=true;}else if(eq(k,"result")){if(res)return ApiResponseError::malformed_json;e=r.integer(&out->result);if(e!=ApiResponseError::none)return e;res=true;}else if(eq(k,"contents")){if(contents)return ApiResponseError::malformed_json;r.ws();const uint8_t* b=r.pos();e=r.value(0);if(e!=ApiResponseError::none)return e;content={b,r.pos()};contents=true;}else{e=r.value(0);if(e!=ApiResponseError::none)return e;}if(r.take('}'))break;if(!r.take(','))return ApiResponseError::malformed_json;}if(!r.done())return ApiResponseError::malformed_json;if(!cmd||!res)return ApiResponseError::missing_required_field;if(!eq(cv,expected))return ApiResponseError::unexpected_command;out->has_contents=contents;out->contents_json=contents?wfrac_protocol::ByteView{content.begin,static_cast<size_t>(content.end-content.begin)}:wfrac_protocol::ByteView{};return ApiResponseError::none;}
ApiResponseError parse_get_device_info_contents(wfrac_protocol::ByteView in,wfrac_protocol::MutableByteView air,wfrac_protocol::MutableByteView mac,GetDeviceInfoContents* out){if(out)*out={};if(!out||(in.size&&!in.data))return ApiResponseError::invalid_argument;if(air.size&&air.data)air.data[0]=0;if(mac.size&&mac.data)mac.data[0]=0;Reader r(in);if(!r.take('{'))return ApiResponseError::invalid_field_type;bool id=false,hm=false,ha=false;if(!r.take('}'))for(;;){Span k,v;auto e=r.string(&k);if(e!=ApiResponseError::none)return e;if(!r.take(':'))return ApiResponseError::malformed_json;if(eq(k,"airconId")){if(id)return ApiResponseError::malformed_json;e=r.string(&v);if(e!=ApiResponseError::none)return e;e=decode(v,air,&out->aircon_id_size);if(e!=ApiResponseError::none){*out={};return e;}id=true;}else if(eq(k,"macAddress")){if(hm)return ApiResponseError::malformed_json;e=r.string(&v);if(e!=ApiResponseError::none)return e;e=decode(v,mac,&out->mac_address_size);if(e!=ApiResponseError::none){*out={};return e;}out->has_mac_address=true;hm=true;}else if(eq(k,"apMode")){if(ha)return ApiResponseError::malformed_json;e=r.integer(&out->ap_mode);if(e!=ApiResponseError::none)return e;out->has_ap_mode=true;ha=true;}else{e=r.value(0);if(e!=ApiResponseError::none)return e;}if(r.take('}'))break;if(!r.take(','))return ApiResponseError::malformed_json;}if(!r.done())return ApiResponseError::malformed_json;if(!id){*out={};return ApiResponseError::missing_required_field;}return ApiResponseError::none;}

static ApiResponseError base64_error(wfrac_protocol::Base64Error error) {
  switch (error) {
    case wfrac_protocol::Base64Error::none:
      return ApiResponseError::none;
    case wfrac_protocol::Base64Error::output_too_small:
      return ApiResponseError::output_too_small;
    case wfrac_protocol::Base64Error::invalid_argument:
      return ApiResponseError::invalid_argument;
    case wfrac_protocol::Base64Error::size_overflow:
      return ApiResponseError::integer_overflow;
    case wfrac_protocol::Base64Error::invalid_length:
    case wfrac_protocol::Base64Error::invalid_character:
    case wfrac_protocol::Base64Error::invalid_padding:
      return ApiResponseError::invalid_base64;
  }
  return ApiResponseError::invalid_base64;
}

ApiResponseError parse_get_aircon_stat_contents(
    wfrac_protocol::ByteView in,
    wfrac_protocol::MutableByteView air,
    wfrac_protocol::MutableByteView scratch,
    wfrac_protocol::MutableByteView raw,
    GetAirconStatContents* out) {
  if (out != nullptr) *out = {};
  if (out == nullptr || (in.size != 0U && in.data == nullptr)) return ApiResponseError::invalid_argument;
  if (air.size != 0U && air.data != nullptr) air.data[0] = 0;
  if (scratch.size != 0U && scratch.data != nullptr) scratch.data[0] = 0;

  Reader reader(in);
  if (!reader.take('{')) return ApiResponseError::invalid_field_type;
  bool has_aircon_stat = false;
  bool has_aircon_id = false;
  Span aircon_stat{};
  Span aircon_id{};
  if (!reader.take('}')) {
    for (;;) {
      Span key{};
      ApiResponseError error = reader.string(&key);
      if (error != ApiResponseError::none) return error;
      if (!reader.take(':')) return ApiResponseError::malformed_json;
      if (eq(key, "airconStat")) {
        if (has_aircon_stat) return ApiResponseError::malformed_json;
        error = reader.string(&aircon_stat);
        if (error != ApiResponseError::none) return error;
        has_aircon_stat = true;
      } else if (eq(key, "airconId")) {
        if (has_aircon_id) return ApiResponseError::malformed_json;
        error = reader.string(&aircon_id);
        if (error != ApiResponseError::none) return error;
        has_aircon_id = true;
      } else {
        error = reader.value(0);
        if (error != ApiResponseError::none) return error;
      }
      if (reader.take('}')) break;
      if (!reader.take(',')) return ApiResponseError::malformed_json;
    }
  }
  if (!reader.done()) return ApiResponseError::malformed_json;
  if (!has_aircon_stat) return ApiResponseError::missing_required_field;

  GetAirconStatContents parsed{};
  if (has_aircon_id) {
    ApiResponseError error = decode(aircon_id, air, &parsed.aircon_id_size);
    if (error != ApiResponseError::none) return error;
    parsed.has_aircon_id = true;
  }

  ApiResponseError error = decode(aircon_stat, scratch, &parsed.encoded_aircon_stat_size);
  if (error != ApiResponseError::none) return error;

  std::size_t raw_size = 0U;
  error = base64_error(wfrac_protocol::base64_decoded_size(
      {scratch.data, parsed.encoded_aircon_stat_size}, &raw_size));
  if (error != ApiResponseError::none) return error;
  if (raw.size < raw_size) return ApiResponseError::output_too_small;
  error = base64_error(wfrac_protocol::base64_decode(
      {scratch.data, parsed.encoded_aircon_stat_size}, raw, &raw_size));
  if (error != ApiResponseError::none) return error;

  wfrac_protocol::AirconFrameView frame{};
  if (wfrac_protocol::parse_aircon_stat({raw.data, raw_size}, &frame) !=
      wfrac_protocol::FrameError::none) {
    return ApiResponseError::invalid_aircon_stat;
  }
  if (!frame.command.crc_valid || !frame.receive.crc_valid) return ApiResponseError::crc_mismatch;

  parsed.raw_aircon_stat_size = raw_size;
  parsed.frame = frame;
  *out = parsed;
  return ApiResponseError::none;
}
}  // namespace wfrac_api
